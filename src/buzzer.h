#pragma once
#include <Arduino.h>

// Buzzerpatronen per ernst uit docs/ble-protocol.md, zodat je het verschil
// hoort zonder te kijken:
//   info         1 korte piep
//   waarschuwing 3x kort, elke 30 s opnieuw tot bevestigen
//   alarm        continu afwisselend hoog/laag tot bevestigen
namespace buzzer {

void begin();

// welk patroon er moet klinken: -1 = stil, anders de ernst (SEV_*)
void set(int severity);

void beep();  // één korte piep, voor info en de test
void update();  // vaak aanroepen

}
