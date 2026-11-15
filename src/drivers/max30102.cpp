// MAX30102: PPG op 100 Hz, hartslag, SpO2 en de chiptemperatuur
#include <MAX30105.h>
#include <Wire.h>
#include <spo2_algorithm.h>

#include "../packets.h"
#include "../sampler.h"
#include "astronav_protocol.h"
#include "drivers.h"
#include "pins.h"

using namespace astronav;

const uint8_t ADDR = 0x57;
const uint8_t REG_FIFO_WR = 0x04, REG_FIFO_RD = 0x06, REG_FIFO_DATA = 0x07;
const uint8_t REG_TEMP_INT = 0x1F, REG_TEMP_CONFIG = 0x21;

const uint32_t NO_CONTACT_IR = 50000;  // lager = geen pols tegen de sensor
const uint32_t SAMPLE_MS = 10;         // 100 Hz
const int DECIMATE = 4;                // naar 25 Hz voor het Maxim-algoritme

static MAX30105 sensor;

// --- sample-task ---
static PpgPacket packet;
static int inPacket = 0;

static uint32_t irRing[BUFFER_SIZE], redRing[BUFFER_SIZE];  // 4 s op 25 Hz
static uint8_t head = 0, filled = 0, sinceCalc = 0;
static uint32_t sumRed = 0, sumIr = 0;
static int summed = 0;

static uint32_t windowIr[BUFFER_SIZE], windowRed[BUFFER_SIZE];
static volatile bool windowReady = false;

static volatile bool contact = false;
static volatile int16_t dieTemp = 0x7FFF;  // 0.01 °C
static uint32_t tempAsked = 0, lastTemp = 0;

// --- loop() ---
static uint8_t hr = 0, spo2 = 0, perfusion = 0;
static uint32_t lastValid = 0;

static uint8_t read8(uint8_t reg) {
  Wire.beginTransmission(ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;
  if (Wire.requestFrom(ADDR, (uint8_t)1) != 1) return 0;
  return Wire.read();
}

static void write8(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

bool max30102Begin() {
  if (!sensor.begin(Wire, pins::I2C_FREQ_HZ)) return false;
  // rood + IR, 100 Hz zonder middelen, 18 bits
  sensor.setup(60, 1, 2, 100, 411, 4096);
  return true;
}

static void addSample(uint32_t red, uint32_t ir, uint32_t t) {
  if (inPacket == 0) packets::header(packet.h, PKT_PPG, t);
  packet.red[inPacket] = red;
  packet.ir[inPacket] = ir;
  if (++inPacket == 10) {
    if (sampler::rawStreams()) sampler::push(&packet, sizeof(packet));
    inPacket = 0;
  }

  contact = ir >= NO_CONTACT_IR;
  if (!contact) {
    filled = summed = sumRed = sumIr = 0;
    return;
  }

  sumRed += red;
  sumIr += ir;
  if (++summed < DECIMATE) return;
  redRing[head] = sumRed / DECIMATE;
  irRing[head] = sumIr / DECIMATE;
  summed = sumRed = sumIr = 0;
  head = (head + 1) % BUFFER_SIZE;
  if (filled < BUFFER_SIZE) filled++;

  // elke seconde het venster van 4 s doorgeven aan loop(), daar wordt gerekend
  if (filled == BUFFER_SIZE && ++sinceCalc >= 25 && !windowReady) {
    sinceCalc = 0;
    for (int i = 0; i < BUFFER_SIZE; i++) {
      int idx = (head + i) % BUFFER_SIZE;  // oudste eerst
      windowIr[i] = irRing[idx];
      windowRed[i] = redRing[idx];
    }
    windowReady = true;
  }
}

void max30102Sample(uint32_t now) {
  // chiptemperatuur elke 10 s: aanzetten, ~30 ms later uitlezen
  if (tempAsked && now - tempAsked >= 40) {
    int8_t whole = read8(REG_TEMP_INT);
    uint8_t frac = read8(REG_TEMP_INT + 1);
    dieTemp = whole * 100 + frac * 625 / 100;
    tempAsked = 0;
    lastTemp = now;
  } else if (!tempAsked && (!lastTemp || now - lastTemp >= 10000)) {
    write8(REG_TEMP_CONFIG, 1);
    tempAsked = now;
  }

  uint8_t wr = read8(REG_FIFO_WR);
  uint8_t rd = read8(REG_FIFO_RD);
  int n = (wr - rd) & 0x1F;
  if (n == 0) return;

  Wire.beginTransmission(ADDR);
  Wire.write(REG_FIFO_DATA);
  if (Wire.endTransmission(false) != 0) return;
  int got = Wire.requestFrom(ADDR, (uint8_t)(n * 6)) / 6;

  for (int k = 0; k < got; k++) {
    uint8_t b[6];
    for (auto &x : b) x = Wire.read();
    uint32_t red = ((uint32_t)b[0] << 16 | b[1] << 8 | b[2]) & 0x3FFFF;
    uint32_t ir = ((uint32_t)b[3] << 16 | b[4] << 8 | b[5]) & 0x3FFFF;
    // de nieuwste sample is van nu, de rest 10 ms per stap eerder
    addSample(red, ir, now - (got - 1 - k) * SAMPLE_MS);
  }
}

void max30102Update() {
  if (!windowReady) return;

  int32_t newSpo2, newHr;
  int8_t spo2Valid, hrValid;
  maxim_heart_rate_and_oxygen_saturation(windowIr, BUFFER_SIZE, windowRed, &newSpo2, &spo2Valid, &newHr, &hrValid);

  // perfusie-index: hoeveel van het IR-signaal pulseert, in %
  uint32_t lo = UINT32_MAX, hi = 0;
  uint64_t sum = 0;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    lo = min(lo, windowIr[i]);
    hi = max(hi, windowIr[i]);
    sum += windowIr[i];
  }
  float mean = (float)sum / BUFFER_SIZE;
  perfusion = constrain((hi - lo) / mean * 1000, 0, 254);  // x10 %

  windowReady = false;

  // het algoritme springt nogal, dus wat uitmiddelen
  if (hrValid && newHr >= 30 && newHr <= 220) {
    hr = hr ? (hr * 3 + newHr) / 4 : newHr;
    lastValid = millis();
  }
  if (spo2Valid && newSpo2 >= 70 && newSpo2 <= 100) {
    spo2 = spo2 ? (spo2 * 3 + newSpo2) / 4 : newSpo2;
    lastValid = millis();
  }
}

void max30102Fill(Telemetry &t) {
  if (dieTemp != 0x7FFF) {
    t.flags |= HAS_SKIN_TEMP;
    t.skin_temp = dieTemp;
  }

  if (!contact) {
    hr = spo2 = 0;
    return;
  }
  t.vitals_flags |= FLAG_SKIN_CONTACT;
  if (!hr || !spo2 || millis() - lastValid > 10000) return;
  t.flags |= HAS_HEART;
  t.heart_rate = hr;
  t.spo2 = spo2;
  t.perfusion_c10 = perfusion;
}
