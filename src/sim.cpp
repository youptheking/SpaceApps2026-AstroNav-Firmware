#include "sim.h"

#include <math.h>

namespace sim {

static float noise(float amount) {
  return random(-1000, 1001) / 1000.0f * amount;
}

static float heartRate(float s) {
  // elke 3 minuten een zuurstofdip van 20 s, hart gaat dan wat sneller
  bool dip = fmodf(s, 180) > 160;
  return 72 + 6 * sinf(s / 40) + (dip ? 15 : 0);
}

void fill(Telemetry &t) {
  float s = millis() / 1000.0f;
  bool dip = fmodf(s, 180) > 160;

  t.flags |= HAS_HEART | HAS_SKIN_TEMP | HAS_AIR | HAS_LIGHT | HAS_IMU | HAS_GSR | ECG_LEADS_OK;
  t.status |= STATUS_SIMULATED;

  t.heart_rate = heartRate(s) + noise(2);
  t.spo2 = dip ? 89 + noise(1.5) : 97.5 + noise(1);
  t.skin_temp = (36.6 + 0.1 * sinf(s / 300) + noise(0.03)) * 100;

  // co2 loopt op tot de scrubber hem elke 5 minuten weer omlaag haalt
  float cycle = fmodf(s, 300) / 300;
  t.co2 = 600 + 1300 * cycle + noise(20);
  t.cabin_temp = (22.5 + noise(0.1)) * 100;
  t.humidity = (45 + 2 * sinf(s / 120)) * 100;
  t.lux = 300 + noise(15);

  // huidgeleiding gaat omhoog bij stress, hier tijdens de dip
  t.gsr = 1200 + (dip ? 400 : 0) + noise(30);

  t.accel[0] = noise(20);
  t.accel[1] = noise(20);
  t.accel[2] = 1000 + noise(20);
  t.activity = 10 + noise(5);

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
