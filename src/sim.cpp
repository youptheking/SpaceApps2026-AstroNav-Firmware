#include "sim.h"

#include <math.h>

#include "astronav_protocol.h"

using namespace astronav;

namespace sim {

// vaste waarden via `sim <veld> <waarde>`, 0 = automatisch
static float fixedCo2, fixedSpo2, fixedHr, fixedTemp, fixedCabin, fixedPressure;

static float noise(float amount) {
  return random(-1000, 1001) / 1000.0f * amount;
}

bool fix(const String &field, float value) {
  if (field == "co2") fixedCo2 = value;
  else if (field == "spo2") fixedSpo2 = value;
  else if (field == "hr") fixedHr = value;
  else if (field == "temp") fixedTemp = value;
  else if (field == "cabin") fixedCabin = value;
  else if (field == "pressure") fixedPressure = value;
  else return false;
  return true;
}

void fill(Telemetry &t, uint8_t which) {
  float s = millis() / 1000.0f;
  // elke 3 minuten een zuurstofdip van 20 s, hart gaat dan wat sneller
  bool dip = fmodf(s, 180) > 160;
  float hr = fixedHr ? fixedHr : 72 + 6 * sinf(s / 40) + (dip ? 15 : 0);

  t.flags |= which;
  t.sim_flags |= which;
  if (which) t.status |= STATUS_SIMULATED;

  if (which & HAS_HEART) {
    t.heart_rate = hr + noise(2);
    t.spo2 = fixedSpo2 ? fixedSpo2 : dip ? 89 + noise(1.5) : 97.5 + noise(1);
    t.perfusion_c10 = 25 + noise(5);
    t.vitals_flags |= FLAG_SKIN_CONTACT;
  }
  if (which & HAS_SKIN_TEMP) {
    float temp = fixedTemp ? fixedTemp : 33.5 + 0.1 * sinf(s / 300) + noise(0.03);
    t.skin_temp = temp * 100;
  }
  if (which & ECG_LEADS_OK) {
    t.hr_ecg = hr;
    t.rmssd = 42 + noise(4);
  }

  if (which & HAS_AIR) {
    // co2 loopt op tot de scrubber hem elke 10 minuten weer omlaag haalt.
    // 1500-3200 ppm = 1,1-2,4 mmHg, dus netjes onder de NASA-grens
    float cycle = fmodf(s, 600) / 600;
    t.co2 = (fixedCo2 ? fixedCo2 : 1500 + 1700 * cycle) + noise(20);
    t.cabin_temp = (fixedCabin ? fixedCabin : 22.5 + noise(0.1)) * 100;
    t.humidity = (45 + 2 * sinf(s / 120)) * 100;
  }
  if (which & HAS_PRESSURE) {
    t.pressure_pa = (fixedPressure ? fixedPressure : 101325) + noise(3);
    t.pressure_trend = noise(1);
  }

  // huidgeleiding (12 µS) gaat omhoog bij stress, hier tijdens de dip
  if (which & HAS_GSR) t.gsr = 1200 + (dip ? 400 : 0) + noise(30);

  if (which & HAS_IMU) {
    t.activity_mg = 10 + noise(5);
    t.activity = 0;
    t.tremor = (dip ? 25 : 5) + noise(3);
    t.tremor_hz_c10 = dip ? 85 : 0;
  }
}

// Cabineverlichting: ~380 lux overdag, elke 15 minuten 3 minuten gedimd tot
// "nacht". Op het ISS zie je 16 zonsopkomsten per dag, de verlichting is daar
// het houvast voor het dag-nachtritme. In het echt zou dit de BH1750 meten.
void fillLight(Telemetry &t) {
  float s = millis() / 1000.0f;
  bool night = fmodf(s, 900) > 720;
  float lux = night ? 6 + noise(1) : 380 + 30 * sinf(s / 50) + noise(8);
  t.lux_c10 = lux * 10;
  t.sim_light = 1;
  t.status |= STATUS_SIMULATED;
}

// accu loopt langzaam leeg van ~85 % naar ~40 % en begint dan opnieuw,
// zodat het accualarm (onder 15 %) in de demo niet afgaat
void fillBattery(Telemetry &t) {
  uint32_t minutes = millis() / 60000;
  t.battery_mv = 4050 - (minutes * 3) % 300;
  t.sim_battery = 1;
  t.status |= STATUS_SIMULATED;
}

}
