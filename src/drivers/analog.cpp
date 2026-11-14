// GSR (huidgeleiding via twee koperen vlakken) en AD8232 (ECG), allebei analoog
#include "drivers.h"
#include "pins.h"

static float conductance = 0;  // µS, 0 = geen contact
static uint32_t lastRead = 0;

bool analogBegin() {
  return pins::ANALOG_FRONTEND;
}

// Op de pcb: 3,3 V -> koperen vlak -> huid -> vlak -> D9, en 220k naar GND.
// V = 3,3 * 220k / (Rhuid + 220k), dus G = V / (220k * (3,3 - V)).
// Meer zweet = lagere huidweerstand = hogere spanning.
static float toMicroSiemens(float mv) {
  const float R_MOHM = 0.22f;
  if (mv < 30) return 0;  // geen huidcontact
  if (mv > 3200) mv = 3200;
  return mv / (R_MOHM * (3300 - mv));
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
