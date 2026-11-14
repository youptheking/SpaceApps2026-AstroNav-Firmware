#pragma once
#include <Arduino.h>

#include "telemetry.h"

// Lokale alarmen, met de codes uit docs/ble-protocol.md. Bit n in
// Telemetry.alerts = eventcode n. Bronnen en grenzen: docs/alarmen.md
namespace alerts {

// wordt aangeroepen bij elk alarm dat begint, eindigt of erger wordt
typedef void (*Changed)(uint8_t code, bool on, uint8_t severity, int16_t value);

// zet t.alerts, t.ppco2 en t.ppco2_avg. Eén keer per seconde aanroepen.
void update(Telemetry &t, Changed changed);

uint8_t severity(uint8_t code);  // van een actief alarm

// korte tekst voor het scherm (max 21 tekens), de belangrijkste eerst.
// nullptr als er niks is
const char *text(uint16_t active);

// het ergste actieve alarm uit `active`, -1 als er geen is
int worst(uint16_t active);

// CO2-gemiddelde opnieuw beginnen, bijv. als de simulatie een andere waarde krijgt
void resetCo2();

}
