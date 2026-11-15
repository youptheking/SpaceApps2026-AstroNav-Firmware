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

// 50 Hz netbrom: op 250 Hz is dat precies 5 samples per periode, dus het
// gemiddelde van de laatste 5 samples haalt hem (en 100 Hz) er volledig uit.
// De QRS (~100 ms breed) blijft staan. Zonder dit zag de R-piekdetector de
// brom als hartslag. Het ECG-pakket blijft ruw, het dashboard filtert zelf.
const int MAINS_PERIOD = 5;
static uint16_t mainsRing[MAINS_PERIOD];
static uint32_t mainsSum = 0;
static int mainsPos = 0, mainsFilled = 0;

// analogReadMilliVolts loopt van 0 tot ~3160 mV; daar zit het signaal vast
const uint16_t ECG_RAIL_LOW_MV = 0, ECG_RAIL_HIGH_MV = 3150;
const int CLIP_BLOCK = 500;       // 2 s op 250 Hz
const int MAX_CLIPPED_PCT = 5;
static int blockCount = 0, blockClipped = 0;
static volatile bool saturated = false;

static float removeMains(uint16_t mv) {
  mainsSum += mv;
  mainsSum -= mainsRing[mainsPos];
  mainsRing[mainsPos] = mv;
  mainsPos = (mainsPos + 1) % MAINS_PERIOD;
  if (mainsFilled < MAINS_PERIOD) mainsFilled++;
  return (float)mainsSum / mainsFilled;
}

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

  // Overstuurd (gezien: 65 % van de samples op 0 of 3,15 V door netbrom): een
  // afgekapt signaal valt niet meer te filteren, de "slagen" zijn dan ruis.
  // Per blok van 2 s kijken; dan geen hartslag, RR of RMSSD in plaats van onzin.
  bool clipped = mv <= ECG_RAIL_LOW_MV || mv >= ECG_RAIL_HIGH_MV;
  blockClipped += clipped;
  if (++blockCount == CLIP_BLOCK) {
    bool wasSaturated = saturated;
    saturated = blockClipped > CLIP_BLOCK * MAX_CLIPPED_PCT / 100;
    if (saturated && !wasSaturated) {
      Serial.printf("ecg: signaal overstuurd (%d%% afgekapt), geen hartslag\n", blockClipped * 100 / CLIP_BLOCK);
      peaks.reset();
      recentCount = 0;
      hr = 0;
      rmssd = 0;
      pendingCount = 0;
    }
    blockCount = blockClipped = 0;
  }
  if (saturated) return;

  uint16_t rr;
  if (peaks.add(removeMains(mv), now, rr)) addInterval(rr, peaks.lastBeat());
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
  if (saturated) return;
  // na 3 s zonder slag geen hartslag meer noemen
  if (millis() - peaks.lastBeat() < 3000) t.hr_ecg = hr;
  t.rmssd = rmssd;
}
