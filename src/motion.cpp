#include "motion.h"

#include <math.h>

// band van 4 tot 12 Hz, daar zitten fysiologische en pathologische tremoren
const float BAND_LOW = 4, BAND_HIGH = 12;
const float TREMOR_WINDOW_S = 2;
const float ACTIVITY_WINDOW_S = 1;
const float ACTIVITY_HIGHPASS_HZ = 0.5;  // langzamer is draaien van de pols, geen beweging
const float MIN_TREMOR_MG = 3;  // daaronder is het ruis, geen frequentie noemen

// schok: meer dan 3 g in totaal (in rust is het 1 g)
const float SHOCK_MG = 3000;
// tik op het horloge: korte piek in de versnelling zonder zwaartekracht.
// Twee tikken tussen 0,12 en 0,5 s uit elkaar = dubbeltik. Afgesteld op het
// prototype, bij een andere behuizing opnieuw proberen.
const float TAP_MG = 700;
const float DOUBLE_TAP_MIN_S = 0.12, DOUBLE_TAP_MAX_S = 0.5;
const float TAP_MAX_ACTIVITY_MG = 120;  // tijdens lopen of sporten niet op tikken letten

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
  n++;

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
    axisPower[i] += (b * b - axisPower[i]) / (fs * TREMOR_WINDOW_S);

    // frequentie uit het aantal nuldoorgangen, per seconde
    bool crossed = (b > 0) != (lastBand[i] > 0);
    crossRate[i] += ((crossed ? fs : 0) - crossRate[i]) / (fs * TREMOR_WINDOW_S);
    lastBand[i] = b;

    hpOut[i] = hpAlpha * (hpOut[i] + a[i] - hpPrev[i]);
    hpPrev[i] = a[i];
    activitySq += hpOut[i] * hpOut[i];

    avg[i] += (a[i] - avg[i]) / fs;
  }

  tremorPower += (tremorSq - tremorPower) / (fs * TREMOR_WINDOW_S);
  float activityBefore = activityMg();
  activityPower += (activitySq - activityPower) / (fs * ACTIVITY_WINDOW_S);

  float total = sqrtf(ax * ax + ay * ay + az * az);
  if (total > SHOCK_MG) {
    if (n - shockAt > fs) shockPeak = 0;
    shockAt = n;
    shockPeak = max(shockPeak, (uint16_t)min(total, 65535.0f));
  }

  // tik = de piek gaat over de drempel; daarna eerst weer onder laten zakken
  float hp = sqrtf(activitySq);
  bool above = hp > TAP_MG;
  if (above && !aboveTap && activityBefore < TAP_MAX_ACTIVITY_MG) {
    float since = (n - tapAt) / fs;
    if (tapAt && since >= DOUBLE_TAP_MIN_S && since <= DOUBLE_TAP_MAX_S) {
      doubleTap = true;
      tapAt = 0;
    } else {
      tapAt = n;
    }
  }
  aboveTap = above;
}

float Motion::tremorMg() const { return sqrtf(tremorPower); }
float Motion::activityMg() const { return sqrtf(activityPower); }

uint8_t Motion::tremor() const { return min(tremorMg(), 100.0f); }
uint8_t Motion::activity() const { return min(activityMg() / 5, 100.0f); }

float Motion::tremorHz() const {
  if (tremorMg() < MIN_TREMOR_MG) return 0;
  int axis = 0;
  for (int i = 1; i < 3; i++) {
    if (axisPower[i] > axisPower[axis]) axis = i;
  }
  return crossRate[axis] / 2;
}

int16_t Motion::mean(int axis) const { return avg[axis]; }

uint16_t Motion::shockMg() const {
  return shockAt && n - shockAt <= fs ? shockPeak : 0;
}

bool Motion::takeDoubleTap() {
  bool was = doubleTap;
  doubleTap = false;
  return was;
}
