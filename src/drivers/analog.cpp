// Grove GSR (huidgeleiding) en AD8232 (ECG), allebei analoog
#include "drivers.h"
#include "pins.h"

static float conductance = 0;  // µS, 0 = geen contact
static uint32_t lastRead = 0;

bool analogBegin() {
  return pins::ANALOG_FRONTEND;
}

// Formule uit de Grove GSR-wiki, omgerekend van een 10-bits waarde op Vcc.
// Meer zweet = lagere spanning = hogere geleiding. De module moet wel met de
// potmeter afgesteld worden (zonder huid ~ halve Vcc), daarna nog nakijken.
static float toMicroSiemens(float mv) {
  float x = mv / 3300.0f * 1024;
  if (x >= 500) return 0;  // geen huidcontact
  float ohm = (1024 + 2 * x) * 10000 / (512 - x);
  return 1e6 / ohm;
}

void analogUpdate() {
  if (millis() - lastRead < 100) return;
  lastRead = millis();

  float us = toMicroSiemens(analogReadMilliVolts(pins::GSR));
  if (us == 0) conductance = 0;
  else conductance = conductance ? conductance + (us - conductance) * 0.2 : us;
}

void analogFill(Telemetry &t) {
  if (conductance > 0) {
    t.flags |= HAS_GSR;
    t.gsr = conductance * 100;
  }

  // LO+ of LO- hoog = elektrode los
  if ((t.status & STATUS_ECG_ON) && !digitalRead(pins::ECG_LO_PLUS) && !digitalRead(pins::ECG_LO_MINUS))
    t.flags |= ECG_LEADS_OK;
}
