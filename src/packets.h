#pragma once
#include <Arduino.h>

#include "astronav_protocol.h"
#include "telemetry.h"

// Pakketten uit docs/ble-protocol.md. Ongeldig = 0 bij velden zonder
// fysieke nul, anders de hoogste waarde van het type.
namespace packets {

const uint8_t FW_MAJOR = 1, FW_MINOR = 0;

// type, versie, volgnummer per type en tijdstip van de eerste sample
void header(astronav::Header &h, uint8_t type, uint32_t t_ms);

astronav::VitalsPacket vitals(const Telemetry &t);
astronav::AirPacket air(const Telemetry &t);
astronav::CabinPacket cabin(const Telemetry &t);
astronav::StatusPacket status(const Telemetry &t, uint8_t sensorsOk);
astronav::EventPacket event(uint8_t code, uint8_t state, uint8_t severity, int16_t value, uint8_t source);

}
