// AD8232: ECG op 250 Hz, R-pieken en RR-intervallen
#include "../packets.h"
#include "../rpeaks.h"
#include "../sampler.h"
#include "astronav_protocol.h"
#include "drivers.h"
#include "pins.h"

using namespace astronav;

const float SAMPLE_RATE = 250;
const int LEADS_DEBOUNCE = 25;  // 100 ms los voordat we het geloven
const int RR_KEEP = 32;         // voor hartslag en RMSSD

static RPeaks peaks(SAMPLE_RATE);
static EcgPacket packet;
static int inPacket = 0;
static int offCount = 0;
static volatile bool leadsOff = true;

// RR-intervallen voor het volgende RR-pakket (max 5 per seconde)
static uint16_t pending[5];
static int pendingCount = 0;
static uint32_t pendingStart = 0;

static uint16_t recent[RR_KEEP];
static int recentCount = 0, recentHead = 0;
static volatile uint8_t hr = 0;
static volatile uint16_t rmssd = 0;

bool ecgBegin() {
  if (!pins::ANALOG_FRONTEND) return false;
  pinMode(pins::ECG_LO_PLUS, INPUT);
  pinMode(pins::ECG_LO_MINUS, INPUT);
  // SDN als ingang, de pull-up op het ad8232-bordje houdt hem hoog (= aan)
  pinMode(pins::ECG_SDN, INPUT);
  return true;
}

static void addInterval(uint16_t rr, uint32_t beat) {
  if (pendingCount == 0) pendingStart = beat - rr;  // de slag waar het interval begint
  if (pendingCount < 5) pending[pendingCount++] = rr;

  recent[recentHead] = rr;
  recentHead = (recentHead + 1) % RR_KEEP;
  if (recentCount < RR_KEEP) recentCount++;

  // hartslag uit de laatste 8 slagen
  int n = min(recentCount, 8);
  uint32_t sum = 0;
  for (int i = 1; i <= n; i++) sum += recent[(recentHead - i + RR_KEEP) % RR_KEEP];
  hr = 60000 * n / sum;

  // RMSSD: wortel uit het gemiddelde van de gekwadrateerde verschillen
  if (recentCount >= 10) {
    float sq = 0;
    for (int i = 1; i < recentCount; i++) {
      int a = recent[(recentHead - i + RR_KEEP) % RR_KEEP];
      int b = recent[(recentHead - i - 1 + RR_KEEP) % RR_KEEP];
      sq += (a - b) * (a - b);
    }
    rmssd = sqrtf(sq / (recentCount - 1));
  }
}

void ecgSample(uint32_t now) {
  uint16_t mv = analogReadMilliVolts(pins::ECG_OUT);

  if (inPacket == 0) packets::header(packet.h, PKT_ECG, now);
  packet.mv[inPacket] = mv;
  if (++inPacket == 25) {
    if (sampler::rawStreams()) sampler::push(&packet, sizeof(packet));
    inPacket = 0;
  }

  // LO+ of LO- hoog = elektrode los
  bool off = digitalRead(pins::ECG_LO_PLUS) || digitalRead(pins::ECG_LO_MINUS);
  offCount = off ? min(offCount + 1, LEADS_DEBOUNCE) : 0;
  bool wasOff = leadsOff;
  leadsOff = offCount >= LEADS_DEBOUNCE;

  if (leadsOff) {
    if (!wasOff) {
      peaks.reset();
      recentCount = 0;
      hr = 0;
      rmssd = 0;
    }
    return;
  }

  uint16_t rr;
  if (peaks.add(mv, now, rr)) addInterval(rr, peaks.lastBeat());
}

void rrSend(uint32_t now) {
  if (!pendingCount) return;
  uint8_t buf[sizeof(Header) + 1 + sizeof(pending)];
  Header h;
  packets::header(h, PKT_RR, pendingStart);
  memcpy(buf, &h, sizeof(h));
  buf[sizeof(h)] = pendingCount;
  memcpy(buf + sizeof(h) + 1, pending, pendingCount * 2);
  sampler::push(buf, sizeof(h) + 1 + pendingCount * 2);
  pendingCount = 0;
}

void ecgFill(Telemetry &t) {
  if (leadsOff) {
    t.vitals_flags |= FLAG_ECG_LEADS_OFF;
    return;
  }
  t.flags |= ECG_LEADS_OK;
  // na 3 s zonder slag geen hartslag meer noemen
  if (millis() - peaks.lastBeat() < 3000) t.hr_ecg = hr;
  t.rmssd = rmssd;
}
