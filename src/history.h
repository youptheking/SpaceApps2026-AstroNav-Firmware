#pragma once
#include <Arduino.h>

#include "telemetry.h"

// Minuutsamenvattingen in flash, zodat het dashboard trends kan maken ook als
// de laptop een tijd niet verbonden was. NASA-STD-3001 Vol. 2 [V2 6001].
// Formaat: "Uitbreidingen op v1" in docs/ble-protocol.md.

struct __attribute__((packed)) HistoryRecord {
  uint8_t version = 2;
  uint32_t seq = 0;        // loopt altijd op, ook na een herstart
  uint32_t unix_time = 0;  // begin van de minuut, 0 = klok was niet gezet
  uint8_t flags = 0;       // welke sensoren deze minuut iets gemeten hebben
  uint8_t status = 0;      // o.a. gesimuleerd
  uint16_t alerts = 0;     // alle alarmen die deze minuut aan zijn geweest

  uint8_t hr_avg = 0, hr_min = 0, hr_max = 0;
  uint8_t spo2_avg = 0, spo2_min = 0;
  int16_t skin_temp = 0;  // 0.01 °C

  // per 5 seconden, NASA Tabel 6.1-1 vraagt CO2 elke 5 s
  uint16_t co2[12] = {};  // ppm
  uint16_t ppco2_avg = 0;  // 0.01 mmHg, 1-uursgemiddelde aan het eind van de minuut

  int16_t cabin_temp = 0;  // 0.01 °C
  uint16_t humidity = 0;   // 0.01 %
  uint16_t pressure = 0;  // 0.1 hPa, cabinedruk
  uint16_t gsr = 0;  // 0.01 µS
  uint8_t activity = 0;  // % van de minuut niet in rust
  uint8_t tremor = 0;    // hoogste waarde
  uint16_t battery_mv = 0;
};

static_assert(sizeof(HistoryRecord) == 58, "docs/ble-protocol.md bijwerken");

namespace history {

const uint32_t CAPACITY = 7 * 24 * 60;  // 7 dagen

bool begin();

// elke tick aanroepen, schrijft zelf een record als de minuut om is
void add(const Telemetry &t);

uint32_t oldest();  // 0 als er nog niks is
uint32_t newest();

bool read(uint32_t seq, HistoryRecord &r);
void wipe();

}
