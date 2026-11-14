#pragma once
#include <Arduino.h>

#include "telemetry.h"

namespace display {

bool begin();
void showPairing(const char *name, uint32_t code);
void showStatus(const Telemetry &t, const String &message);

// level 0..1, de cirkel groeit mee met de ademhaling
void showBreathing(const char *text, float level);

}
