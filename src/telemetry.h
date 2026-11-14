#pragma once
#include <Arduino.h>

// Pakket dat elke seconde (of `rate`) naar de laptop gaat.
// Little-endian, packed. Bij een wijziging version ophogen en docs/ble.md bijwerken.

enum SensorFlag : uint8_t {
  HAS_HEART = 1 << 0,  // hartslag + spo2
  HAS_SKIN_TEMP = 1 << 1,
  HAS_AIR = 1 << 2,  // co2, cabinetemp, luchtvochtigheid
  HAS_LIGHT = 1 << 3,
  HAS_IMU = 1 << 4,
  HAS_GSR = 1 << 5,
  ECG_LEADS_OK = 1 << 6,
  HAS_TIME = 1 << 7,  // klok gezet via `time`
};

enum Alert : uint16_t {
  ALERT_LOW_SPO2 = 1 << 0,
  ALERT_HIGH_HR = 1 << 1,
  ALERT_LOW_HR = 1 << 2,
  ALERT_FEVER = 1 << 3,
  ALERT_HIGH_CO2 = 1 << 4,
  ALERT_STRESS = 1 << 5,
  ALERT_LOW_BATT = 1 << 6,
  ALERT_LEADS_OFF = 1 << 7,
};

struct __attribute__((packed)) Telemetry {
  uint8_t version = 1;
  uint8_t flags = 0;
  uint16_t alerts = 0;
  uint32_t uptime_ms = 0;
  uint32_t unix_time = 0;

  uint8_t heart_rate = 0;  // bpm
  uint8_t spo2 = 0;        // %
  int16_t skin_temp = 0;   // 0.01 °C

  uint16_t co2 = 0;         // ppm
  int16_t cabin_temp = 0;   // 0.01 °C
  uint16_t humidity = 0;    // 0.01 %
  uint16_t lux = 0;

  uint16_t gsr = 0;       // mV op de adc
  int16_t accel[3] = {};  // mg
  uint8_t activity = 0;   // 0-100

  uint16_t battery_mv = 0;
};

static_assert(sizeof(Telemetry) == 35, "docs/ble.md bijwerken");
