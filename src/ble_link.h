#pragma once
#include <Arduino.h>

#include "astronav_protocol.h"

// Bluetooth volgens docs/ble-protocol.md: data (notify), events (indicate),
// control (write) en history (notify, uitbreiding).
namespace ble {

void begin();

// true zodra de app verbonden is en de goede code gestuurd heeft (control 0x13)
bool connected();
bool linked();  // verbonden, ook als hij nog op slot zit

uint32_t passkey();
const char *name();

void sendData(const void *packet, size_t len);
void sendEvent(const astronav::EventPacket &e);
void sendHistory(const uint8_t *data, size_t len);

// hoeveel bytes er in één notify passen (MTU - 3)
size_t maxPayload();

// haalt een binnengekomen control-commando op, false als er niks is
bool readControl(uint8_t *buf, size_t &len);

}
