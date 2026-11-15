#pragma once
#include <Arduino.h>

#include "telemetry.h"

// Nepwaarden om alarmen en scherm zonder (alle) sensoren te testen.
// Aan in de demomodus (DEMO_MODE in pins.h, `sim on/off` via usb of control
// 0x12). Wat gesimuleerd is, staat in het SIM-pakket, zodat het dashboard het
// kan laten zien. Een echte sensor gaat altijd voor.
namespace sim {

// vult alleen de sensorgroepen in `which` (bits uit SensorFlag)
void fill(Telemetry &t, uint8_t which);

// licht en accu, die op het prototype ontbreken (alleen in de demomodus)
void fillLight(Telemetry &t);
void fillBattery(Telemetry &t);

// vaste waarde voor co2, spo2, hr, temp, cabin of pressure. 0 = weer automatisch.
bool fix(const String &field, float value);

}
