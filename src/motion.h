#pragma once
#include <Arduino.h>

// Activiteit, tremor, schok en dubbeltik uit de versnelling, los van de IMU
// zodat het te testen is (hw_tests/motion). Per as, want een trilling dwars
// op de zwaartekracht zie je nauwelijks terug in de lengte van de vector.
class Motion {
 public:
  explicit Motion(float sampleRate);

  void add(float ax, float ay, float az);  // mg

  uint8_t activity() const;  // 0-100, 1 punt per 5 mg beweging (rms)
  uint8_t tremor() const;    // 0-100, 1 punt per mg in de 4-12 Hz band (rms)
  float tremorMg() const;
  float activityMg() const;
  float tremorHz() const;  // dominante frequentie, 0 als er geen tremor is
  int16_t mean(int axis) const;  // mg, ~1 s gemiddeld

  // piek van de laatste schok in mg, 0 als die langer dan 1 s geleden is
  uint16_t shockMg() const;

  // true als er sinds de vorige keer vragen een dubbeltik was
  bool takeDoubleTap();

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
  float axisPower[3] = {};
  float crossRate[3] = {};  // nuldoorgangen per seconde in de tremorband
  float lastBand[3] = {};
  bool started = false;

  uint32_t n = 0;  // aantal samples, als klok voor schok en tik
  uint32_t shockAt = 0;
  uint16_t shockPeak = 0;
  uint32_t tapAt = 0;
  bool aboveTap = false;
  bool doubleTap = false;
};
