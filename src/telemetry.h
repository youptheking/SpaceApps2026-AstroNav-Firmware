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

// zie docs/alarmen.md voor grenzen en bronnen
enum Alert : uint16_t {
  ALERT_LOW_SPO2 = 1 << 0,
  ALERT_HIGH_HR = 1 << 1,
  ALERT_LOW_HR = 1 << 2,
  ALERT_FEVER = 1 << 3,
  ALERT_HIGH_CO2 = 1 << 4,  // [V2 6004] 1-uursgemiddelde > 3 mmHg
  ALERT_STRESS = 1 << 5,
  ALERT_LOW_BATT = 1 << 6,
  ALERT_LEADS_OFF = 1 << 7,
  ALERT_CO2_RISING = 1 << 8,  // 1-uursgemiddelde > 2,5 mmHg
  ALERT_CABIN = 1 << 9,       // [V2 6012] temp/vochtigheid buiten werkgebied
};

enum Status : uint8_t {
  STATUS_SIMULATED = 1 << 0,  // waarden komen uit sim.cpp, niet van sensoren
  STATUS_BREATHING = 1 << 1,  // ademhalingsoefening loopt
  STATUS_ECG_ON = 1 << 2,
  STATUS_MUTED = 1 << 3,  // buzzer stil na `ack`
};

struct __attribute__((packed)) Telemetry {
  uint8_t version = 5;
  uint8_t flags = 0;
  uint16_t alerts = 0;
  uint32_t uptime_ms = 0;
  uint32_t unix_time = 0;

  uint8_t heart_rate = 0;  // bpm
  uint8_t spo2 = 0;        // %
  int16_t skin_temp = 0;   // 0.01 °C

  uint16_t co2 = 0;         // ppm
  uint16_t ppco2_avg = 0;   // 0.01 mmHg, gemiddelde over het laatste uur
  int16_t cabin_temp = 0;   // 0.01 °C
  uint16_t humidity = 0;    // 0.01 %
  uint16_t lux = 0;

  uint16_t gsr = 0;       // mV op de adc
  int16_t accel[3] = {};  // mg
  uint8_t activity = 0;   // 0-100
  uint8_t tremor = 0;     // 0-100, trilling in de 4-12 Hz band

  uint16_t battery_mv = 0;
  uint8_t status = 0;
  uint8_t sim_flags = 0;  // welke sensorgroepen gesimuleerd zijn (bits uit SensorFlag)
};

static_assert(sizeof(Telemetry) == 40, "docs/ble.md bijwerken");
