// MAX30102: hartslag en SpO2, optisch tegen de huid
#include <MAX30105.h>
#include <Wire.h>
#include <spo2_algorithm.h>

#include "drivers.h"

const uint32_t NO_CONTACT_IR = 50000;  // lager = geen pols tegen de sensor

static MAX30105 sensor;

// laatste 4 s, ringbuffer
static uint32_t irRing[BUFFER_SIZE], redRing[BUFFER_SIZE];
static uint8_t head = 0;
static uint8_t filled = 0;
static uint8_t sinceCalc = 0;

static uint8_t hr = 0, spo2 = 0;
static uint32_t lastValid = 0;
static bool contact = false;
static uint32_t lastCheck = 0;

bool max30102Begin() {
  if (!sensor.begin(Wire, I2C_SPEED_FAST)) return false;
  // rood + IR, 100 Hz met 4x middelen = 25 Hz, wat het Maxim-algoritme verwacht (FreqS)
  sensor.setup(60, 4, 2, 100, 411, 4096);
  return true;
}

static void calculate() {
  static uint32_t ir[BUFFER_SIZE], red[BUFFER_SIZE];
  for (int i = 0; i < BUFFER_SIZE; i++) {
    int idx = (head + i) % BUFFER_SIZE;  // oudste eerst
    ir[i] = irRing[idx];
    red[i] = redRing[idx];
  }

  int32_t newSpo2, newHr;
  int8_t spo2Valid, hrValid;
  maxim_heart_rate_and_oxygen_saturation(ir, BUFFER_SIZE, red, &newSpo2, &spo2Valid, &newHr, &hrValid);

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

void max30102Update() {
  // fifo houdt 32 samples (1,3 s), elke 100 ms kijken is ruim genoeg
  if (millis() - lastCheck < 100) return;
  lastCheck = millis();

  sensor.check();
  while (sensor.available()) {
    uint32_t red = sensor.getFIFORed();
    uint32_t ir = sensor.getFIFOIR();
    sensor.nextSample();

    if (ir < NO_CONTACT_IR) {
      contact = false;
      filled = 0;
      hr = spo2 = 0;
      continue;
    }
    contact = true;

    irRing[head] = ir;
    redRing[head] = red;
    head = (head + 1) % BUFFER_SIZE;
    if (filled < BUFFER_SIZE) filled++;

    // elke seconde opnieuw rekenen over de laatste 4 s
    if (filled == BUFFER_SIZE && ++sinceCalc >= 25) {
      sinceCalc = 0;
      calculate();
    }
  }
}

void max30102Fill(Telemetry &t) {
  if (!contact || !hr || !spo2 || millis() - lastValid > 10000) return;
  t.flags |= HAS_HEART;
  t.heart_rate = hr;
  t.spo2 = spo2;
}
