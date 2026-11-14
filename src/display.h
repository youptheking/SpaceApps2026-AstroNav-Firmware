#pragma once
#include <Arduino.h>

#include "telemetry.h"

namespace display {

bool begin();
// alert = korte alarmtekst onderin, of nullptr
void showPairing(const char *name, uint32_t code, const char *alert);
void showStatus(const Telemetry &t, const String &message, const char *alert);

// level 0..1, de cirkel groeit mee met de ademhaling
void showBreathing(const char *text, float level);

}
