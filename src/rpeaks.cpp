#include "rpeaks.h"

// na een slag kan er 250 ms geen nieuwe komen (max 240 bpm)
const uint32_t REFRACTORY_MS = 250;
const uint16_t RR_MIN = 300, RR_MAX = 2000;  // 30-200 bpm
const float LEARN_S = 2;  // eerst even het niveau van het signaal leren

RPeaks::RPeaks(float sampleRate) : fs(sampleRate) {
  window = min((int)(0.12 * fs), 40);
}

void RPeaks::reset() {
  *this = RPeaks(fs);
}

bool RPeaks::add(float mv, uint32_t t_ms, uint16_t &rr) {
  samples++;

  // langzame basislijn eraf (ademhaling, bewegen)
  if (samples == 1) baseline = mv;
  baseline += (mv - baseline) * 2 / fs;
  float x = mv - baseline;

  // afgeleide over 2 samples, dan kwadraat: de steile QRS springt eruit
  float d = x - prev[1];
  prev[1] = prev[0];
  prev[0] = x;

  sum -= squares[pos];
  squares[pos] = d * d;
  sum += squares[pos];
  pos = (pos + 1) % window;
  float level = sum / window;

  if (samples < LEARN_S * fs) {
    signalLevel = max(signalLevel, level);
    return false;
  }

  float threshold = noiseLevel + 0.25 * (signalLevel - noiseLevel);

  if (level > threshold) {
    if (!inPeak || level > peakValue) {
      peakValue = level;
      peakAt = t_ms;
    }
    inPeak = true;
    return false;
  }

  noiseLevel += (level - noiseLevel) * 0.002;
  if (!inPeak) return false;
  inPeak = false;

  if (beatAt && peakAt - beatAt < REFRACTORY_MS) return false;
  signalLevel += (peakValue - signalLevel) * 0.125;

  uint32_t previous = beatAt;
  beatAt = peakAt;
  if (!previous) return false;

  uint32_t interval = beatAt - previous;
  if (interval < RR_MIN || interval > RR_MAX) return false;
  rr = interval;
  return true;
}
