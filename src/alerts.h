#pragma once
#include <Arduino.h>

#include "telemetry.h"

// Lokale alarmen. Bronnen en grenswaarden: docs/alarmen.md
namespace alerts {

// De SCD40 meet in ppm, de norm rekent in mmHg. ISS-cabine = 1 atm = 760 mmHg.
// Bij een andere cabinedruk (bijv. Artemis) dit aanpassen.
constexpr float CABIN_PRESSURE_MMHG = 760.0;

// zet t.alerts en t.ppco2_avg, één keer per seconde aanroepen
void update(Telemetry &t);

// korte tekst voor het scherm (max 21 tekens), nullptr als er niks is
const char *text(uint16_t active);

// alarmen waarbij de buzzer gaat
bool critical(uint16_t active);

// CO2-gemiddelde opnieuw beginnen, bijv. als de simulatie een andere waarde krijgt
void resetCo2();

}
