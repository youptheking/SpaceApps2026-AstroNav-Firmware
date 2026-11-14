#include "sim.h"

#include <math.h>

namespace sim {

// vaste waarden via `sim <veld> <waarde>`, 0 = automatisch
static float fixedCo2, fixedSpo2, fixedHr, fixedTemp, fixedCabin;

static float noise(float amount) {
  return random(-1000, 1001) / 1000.0f * amount;
}

bool fix(const String &field, float value) {
  if (field == "co2") fixedCo2 = value;
  else if (field == "spo2") fixedSpo2 = value;
  else if (field == "hr") fixedHr = value;
  else if (field == "temp") fixedTemp = value;
  else if (field == "cabin") fixedCabin = value;
  else return false;
  return true;
}

static float heartRate(float s) {
  // elke 3 minuten een zuurstofdip van 20 s, hart gaat dan wat sneller
  bool dip = fmodf(s, 180) > 160;
  return 72 + 6 * sinf(s / 40) + (dip ? 15 : 0);
}

void fill(Telemetry &t, uint8_t which) {
  float s = millis() / 1000.0f;
  bool dip = fmodf(s, 180) > 160;

  t.flags |= which;
  t.sim_flags |= which;
  if (which) t.status |= STATUS_SIMULATED;

  if (which & HAS_HEART) {
    t.heart_rate = fixedHr ? fixedHr : heartRate(s) + noise(2);
    t.spo2 = fixedSpo2 ? fixedSpo2 : dip ? 89 + noise(1.5) : 97.5 + noise(1);
  }
  if (which & HAS_SKIN_TEMP) {
    float temp = fixedTemp ? fixedTemp : 36.6 + 0.1 * sinf(s / 300) + noise(0.03);
    t.skin_temp = temp * 100;
  }

  if (which & HAS_AIR) {
    // co2 loopt op tot de scrubber hem elke 10 minuten weer omlaag haalt.
    // 1500-3200 ppm = 1,1-2,4 mmHg, dus netjes onder de NASA-grens
    float cycle = fmodf(s, 600) / 600;
    t.co2 = (fixedCo2 ? fixedCo2 : 1500 + 1700 * cycle) + noise(20);
    t.cabin_temp = (fixedCabin ? fixedCabin : 22.5 + noise(0.1)) * 100;
    t.humidity = (45 + 2 * sinf(s / 120)) * 100;
  }

  // huidgeleiding (12 µS) gaat omhoog bij stress, hier tijdens de dip
  if (which & HAS_GSR) t.gsr = 1200 + (dip ? 400 : 0) + noise(30);

  if (which & HAS_IMU) {
    t.accel[0] = noise(20);
    t.accel[1] = noise(20);
    t.accel[2] = 1000 + noise(20);
    t.activity = 10 + noise(5);
    t.tremor = (dip ? 25 : 5) + noise(3);
  }

  if (which & ECG_LEADS_OK) t.flags |= ECG_LEADS_OK;
}

void fillBattery(Telemetry &t) {
  t.battery_mv = 3950 - (millis() / 60000) % 400;
}

static float bump(float x, float center, float width, float height) {
  float d = (x - center) / width;
  return height * expf(-d * d);
}

int16_t ecg(uint32_t us) {
  double s = us / 1e6;
  float period = 60.0f / heartRate(s);
  float x = fmod(s, period) / period;  // 0..1 binnen een hartslag

  float v = bump(x, 0.20, 0.025, 0.10)    // P
          + bump(x, 0.37, 0.008, -0.10)   // Q
          + bump(x, 0.40, 0.010, 1.00)    // R
          + bump(x, 0.43, 0.008, -0.20)   // S
          + bump(x, 0.65, 0.040, 0.30);   // T

  return 1650 + v * 600 + noise(8);
}

}
