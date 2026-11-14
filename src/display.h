#pragma once
#include <Arduino.h>

#include "telemetry.h"

namespace display {

bool begin();
void showPairing(const char *name, uint32_t code);
void showStatus(const Telemetry &t, const String &message);

}
