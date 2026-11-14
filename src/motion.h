#pragma once
#include <Arduino.h>

// Activiteit en tremor uit de versnelling, los van de IMU zodat het te testen is
// (hw_tests/motion). Per as, want een trilling dwars op de zwaartekracht zie je
// nauwelijks terug in de lengte van de vector.
class Motion {
 public:
  explicit Motion(float sampleRate);

  void add(float ax, float ay, float az);  // mg

  uint8_t activity() const;  // 0-100, 1 punt per 5 mg beweging (rms)
  uint8_t tremor() const;    // 0-100, 1 punt per mg in de 4-12 Hz band (rms)
  float tremorMg() const;
  float activityMg() const;
  int16_t mean(int axis) const;  // mg, ~1 s gemiddeld

  struct Biquad {
    float b0, b1, b2, a1, a2;
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    float step(float x);
  };

 private:
  float fs;
  Biquad band[3][3];  // per as: twee keer hoogdoorlaat, een keer laagdoorlaat
  float hpPrev[3] = {}, hpOut[3] = {};
  float hpAlpha;
  float avg[3] = {};
  float tremorPower = 0, activityPower = 0;
  bool started = false;
};
