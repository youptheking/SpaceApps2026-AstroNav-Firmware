#pragma once
#include <Arduino.h>

// Laatste stand van alle metingen, één keer per seconde ververst. Dit gaat
// niet zelf over bluetooth: ble_link.cpp maakt er de pakketten uit
// docs/ble-protocol.md van. Alarmen, scherm en geschiedenis lezen dit ook.

enum SensorFlag : uint8_t {
  HAS_HEART = 1 << 0,  // hartslag + spo2 uit de PPG
  HAS_SKIN_TEMP = 1 << 1,
  HAS_AIR = 1 << 2,       // co2, cabinetemp, luchtvochtigheid
  HAS_PRESSURE = 1 << 3,  // cabinedruk van de BMP180
  HAS_IMU = 1 << 4,
  HAS_GSR = 1 << 5,
  ECG_LEADS_OK = 1 << 6,
  HAS_TIME = 1 << 7,  // klok gezet
};

enum Status : uint8_t {
  STATUS_SIMULATED = 1 << 0,  // een deel komt uit sim.cpp
  STATUS_BREATHING = 1 << 1,  // ademhalingsoefening loopt
  STATUS_MUTED = 1 << 3,      // buzzer stil na bevestigen
};

struct Telemetry {
  uint8_t flags = 0;
  uint8_t sim_flags = 0;  // welke groepen gesimuleerd zijn
  uint8_t sim_light = 0;  // lux gesimuleerd (de BH1750 is geschrapt)
  uint8_t sim_battery = 0;  // accu gesimuleerd (prototype draait op usb)
  uint16_t alerts = 0;    // bit n = eventcode n
  uint32_t uptime_ms = 0;
  uint32_t unix_time = 0;

  uint8_t heart_rate = 0;  // bpm, uit de PPG
  uint8_t hr_ecg = 0;      // bpm, uit de R-pieken
  uint8_t spo2 = 0;        // %
  uint8_t perfusion_c10 = 0;
  int16_t skin_temp = 0;  // 0.01 °C, chiptemperatuur van de MAX30102
  uint16_t rmssd = 0;     // ms, uit de RR-intervallen

  uint16_t co2 = 0;        // ppm
  uint16_t ppco2 = 0;      // 0.01 mmHg, nu
  uint16_t ppco2_avg = 0;  // 0.01 mmHg, gemiddelde over het laatste uur
  int16_t cabin_temp = 0;  // 0.01 °C
  uint16_t humidity = 0;   // 0.01 %
  uint32_t pressure_pa = 0;
  int16_t pressure_trend = 0;  // Pa/min
  uint32_t lux_c10 = 0;        // alleen in de demomodus

  uint16_t gsr = 0;  // huidgeleiding in 0.01 µS

  uint8_t activity = 0;        // 0 = rust, 1 = licht, 2 = intensief
  uint16_t activity_mg = 0;    // rms boven 0,5 Hz
  uint8_t tremor = 0;          // 0-255, rms in de 4-12 Hz band in mg
  uint8_t tremor_hz_c10 = 0;   // dominante frequentie, 0 = geen tremor
  uint8_t vitals_flags = 0;    // FLAG_* uit astronav_protocol.h
  uint16_t shock_mg = 0;       // piek van de laatste schok

  uint16_t battery_mv = 0;  // 0 = geen accu
  uint8_t battery_pct = 0;
  uint8_t status = 0;
};
