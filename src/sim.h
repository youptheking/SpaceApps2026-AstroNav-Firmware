#pragma once
#include <Arduino.h>

#include "telemetry.h"

// Nepwaarden om alarmen en scherm zonder (alle) sensoren te testen.
// Staat standaard uit, aanzetten via usb met `sim on`.
namespace sim {

// vult alleen de sensorgroepen in `which` (bits uit SensorFlag)
void fill(Telemetry &t, uint8_t which);

// vaste waarde voor co2, spo2, hr, temp, cabin of pressure. 0 = weer automatisch.
bool fix(const String &field, float value);

}
