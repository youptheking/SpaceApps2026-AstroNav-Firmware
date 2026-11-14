#pragma once
#include <Arduino.h>

// R-pieken in de ECG, een uitgeklede Pan-Tompkins: afgeleide, kwadraat,
// bewegend venster en een drempel die zich aanpast aan het signaal.
class RPeaks {
 public:
  explicit RPeaks(float sampleRate);

  // true als deze sample een nieuwe slag afsluit, `rr` is dan het interval in ms
  bool add(float mv, uint32_t t_ms, uint16_t &rr);

  // tijdstip van de laatst gevonden R-piek
  uint32_t lastBeat() const { return beatAt; }

  void reset();

 private:
  float fs;
  int window;  // samples in het integratievenster (~120 ms)
  float baseline = 0;
  float prev[2] = {};
  float squares[40] = {};
  int pos = 0;
  float sum = 0;

  float signalLevel = 0, noiseLevel = 0;
  uint32_t samples = 0;

  bool inPeak = false;
  float peakValue = 0;
  uint32_t peakAt = 0;
  uint32_t beatAt = 0;
};
