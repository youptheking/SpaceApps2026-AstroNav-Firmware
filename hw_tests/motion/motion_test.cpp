// Test voor motion.cpp zonder IMU: voedt nepsignalen en print de scores.
// pio run -e test_motion -t upload && pio device monitor
#include <Arduino.h>

#include "motion.h"

const float FS = 50;

void run(const char *name, float (*signal)(float t, int axis)) {
  Motion m(FS);
  for (int i = 0; i < FS * 10; i++) {
    float t = i / FS;
    m.add(signal(t, 0), signal(t, 1), signal(t, 2));
  }
  Serial.printf("%-28s tremor %5.1f mg (%3u)  activiteit %6.1f mg (%3u)  gem z %d\n", name,
                m.tremorMg(), m.tremor(), m.activityMg(), m.activity(), m.mean(2));
}

float noise() { return random(-1000, 1001) / 1000.0f * 3; }

float still(float t, int axis) { return (axis == 2 ? 1000 : 0) + noise(); }

// 8 Hz, 50 mg amplitude op x = 35 mg rms
float tremor8hz(float t, int axis) {
  return still(t, axis) + (axis == 0 ? 50 * sinf(2 * PI * 8 * t) : 0);
}

// lopen: 1,5 Hz, 300 mg op y
float walking(float t, int axis) {
  return still(t, axis) + (axis == 1 ? 300 * sinf(2 * PI * 1.5f * t) : 0);
}

// pols langzaam draaien: zwaartekracht schuift van z naar x
float turning(float t, int axis) {
  float angle = 0.3f * sinf(2 * PI * 0.1f * t);
  if (axis == 0) return 1000 * sinf(angle) + noise();
  if (axis == 2) return 1000 * cosf(angle) + noise();
  return noise();
}

void setup() {
  Serial.begin(115200);
  delay(3000);
}

void loop() {
  Serial.println("\nmotion test (10 s per signaal, 50 Hz)");
  run("stil", still);
  run("tremor 8 Hz, 35 mg rms", tremor8hz);
  run("lopen 1,5 Hz, 212 mg rms", walking);
  run("pols draaien 0,1 Hz", turning);
  delay(5000);
}
