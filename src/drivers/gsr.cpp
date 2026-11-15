// GSR: huidgeleiding via twee koperen vlakken aan de onderkant, 10 Hz
#include "../packets.h"
#include "../sampler.h"
#include "astronav_protocol.h"
#include "drivers.h"
#include "pins.h"

using namespace astronav;

static GsrPacket packet;
static int inPacket = 0;
static volatile float conductance = 0;  // µS, 0 = geen contact

bool gsrBegin() {
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

void gsrSample(uint32_t now) {
  float us = toMicroSiemens(analogReadMilliVolts(pins::GSR));

  if (inPacket == 0) packets::header(packet.h, PKT_GSR, now);
  packet.nS[inPacket] = min(us * 1000, 65534.0f);
  if (++inPacket == 10) {
    if (sampler::rawStreams()) sampler::push(&packet, sizeof(packet));
    inPacket = 0;
  }

  if (us == 0) conductance = 0;
  else conductance = conductance ? conductance + (us - conductance) * 0.2f : us;
}

void gsrFill(Telemetry &t) {
  if (conductance <= 0) return;
  t.flags |= HAS_GSR;
  t.gsr = conductance * 100;
}
