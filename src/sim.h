#pragma once
#include <Arduino.h>

#include "telemetry.h"

// Nepwaarden zodat het dashboard en de alarmen zonder sensoren te testen zijn.
namespace sim {

void fill(Telemetry &t);

// ECG-sample in mV, zoals de AD8232 hem zou geven
int16_t ecg(uint32_t us);

// vaste waarde voor co2, spo2, hr, temp of cabin. 0 = weer automatisch.
bool fix(const String &field, float value);

}
