#pragma once
#include <Arduino.h>

#include "telemetry.h"

namespace ble {

void begin();

// true zodra de laptop verbonden is en de koppeling gelukt is
bool connected();

uint32_t passkey();
const char *name();

void send(const Telemetry &t);
void sendEcg(const int16_t *samples, size_t count);
void sendHistory(const uint8_t *data, size_t len);
void reply(const char *text);

// hoeveel bytes er in één notify passen (MTU - 3)
size_t maxPayload();

// haalt een binnengekomen commando op, false als er niks is
bool readCommand(String &out);

}
