#include "motion.h"

#include <math.h>

// band van 4 tot 12 Hz, daar zitten fysiologische en pathologische tremoren
const float BAND_LOW = 4, BAND_HIGH = 12;
const float TREMOR_WINDOW_S = 2;
const float ACTIVITY_WINDOW_S = 1;
const float ACTIVITY_HIGHPASS_HZ = 0.5;  // langzamer is draaien van de pols, geen beweging

float Motion::Biquad::step(float x) {
  float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
  x2 = x1;
  x1 = x;
  y2 = y1;
  y1 = y;
  return y;
}

// RBJ audio EQ cookbook
static void design(Motion::Biquad &b, bool highpass, float f, float q, float fs) {
  float w0 = 2 * PI * f / fs;
  float c = cosf(w0);
  float alpha = sinf(w0) / (2 * q);
  float a0 = 1 + alpha;
  float k = highpass ? (1 + c) / 2 : (1 - c) / 2;
  b.b0 = k / a0;
  b.b1 = (highpass ? -2 * k : 2 * k) / a0;
  b.b2 = k / a0;
  b.a1 = -2 * c / a0;
  b.a2 = (1 - alpha) / a0;
}

Motion::Motion(float sampleRate) : fs(sampleRate) {
  // 4e orde Butterworth hoogdoorlaat (lopen op 1-2 Hz moet er echt uit)
  // + 2e orde laagdoorlaat
  for (int i = 0; i < 3; i++) {
    design(band[i][0], true, BAND_LOW, 0.5412, fs);
    design(band[i][1], true, BAND_LOW, 1.3066, fs);
    design(band[i][2], false, BAND_HIGH, 0.7071, fs);
  }

  float rc = 1 / (2 * PI * ACTIVITY_HIGHPASS_HZ);
  hpAlpha = rc / (rc + 1 / fs);
}

void Motion::add(float ax, float ay, float az) {
  float a[3] = {ax, ay, az};

  if (!started) {
    // filters op de huidige stand laten beginnen, anders eerst een grote piek
    for (int i = 0; i < 3; i++) {
      avg[i] = a[i];
      hpPrev[i] = a[i];
      band[i][0].x1 = band[i][0].x2 = a[i];
    }
    started = true;
  }

  float tremorSq = 0, activitySq = 0;
  for (int i = 0; i < 3; i++) {
    float b = a[i];
    for (auto &stage : band[i]) b = stage.step(b);
    tremorSq += b * b;

    hpOut[i] = hpAlpha * (hpOut[i] + a[i] - hpPrev[i]);
    hpPrev[i] = a[i];
    activitySq += hpOut[i] * hpOut[i];

    avg[i] += (a[i] - avg[i]) / fs;
  }

  tremorPower += (tremorSq - tremorPower) / (fs * TREMOR_WINDOW_S);
  activityPower += (activitySq - activityPower) / (fs * ACTIVITY_WINDOW_S);
}

float Motion::tremorMg() const { return sqrtf(tremorPower); }
float Motion::activityMg() const { return sqrtf(activityPower); }

uint8_t Motion::tremor() const { return min(tremorMg(), 100.0f); }
uint8_t Motion::activity() const { return min(activityMg() / 5, 100.0f); }

int16_t Motion::mean(int axis) const { return avg[axis]; }
