// BMP180: cabinedruk, voor ppCO2 in mmHg en om een lek te zien.
// NASA-STD-3001 [V2 6022]: waarschuwen als de cabinedruk buiten veilige grenzen komt.
#include <Adafruit_BMP085.h>
#include <Wire.h>

#include "drivers.h"

const int TREND_SAMPLES = 60;  // 1 per seconde, trend over de laatste minuut

static Adafruit_BMP085 bmp;
static int32_t history[TREND_SAMPLES];
static int count = 0, head = 0;
static int32_t pressure = 0;
static float trend = 0;  // Pa/min
static uint32_t lastRead = 0, lastPush = 0;

bool bmp180Begin() {
  return bmp.begin(BMP085_ULTRAHIGHRES, &Wire);
}

// helling van een rechte lijn door de laatste minuut, minder last van ruis
// (~3 Pa per meting) dan het verschil tussen begin en eind
static float slopePerMinute() {
  if (count < TREND_SAMPLES / 2) return 0;
  float sx = 0, sy = 0, sxx = 0, sxy = 0;
  for (int i = 0; i < count; i++) {
    float x = i;
    float y = history[(head - count + i + TREND_SAMPLES) % TREND_SAMPLES] - history[(head - count + TREND_SAMPLES) % TREND_SAMPLES];
    sx += x;
    sy += y;
    sxx += x * x;
    sxy += x * y;
  }
  float d = count * sxx - sx * sx;
  return d ? (count * sxy - sx * sy) / d * 60 : 0;
}

void bmp180Update() {
  if (millis() - lastRead < 1000) return;
  lastRead = millis();

  int32_t p = bmp.readPressure();
  if (p < 30000 || p > 120000) return;  // onzin, bus even verstoord
  pressure = p;

  history[head] = p;
  head = (head + 1) % TREND_SAMPLES;
  if (count < TREND_SAMPLES) count++;
  trend = slopePerMinute();

  // de SCD41 rekent zijn ppm om met de druk, één keer per minuut is genoeg
  if (!lastPush || millis() - lastPush > 60000) {
    lastPush = millis();
    scd41SetPressure(p);
  }
}

void bmp180Fill(Telemetry &t) {
  if (!pressure || millis() - lastRead > 5000) return;
  t.flags |= HAS_PRESSURE;
  t.pressure_pa = pressure;
  t.pressure_trend = constrain(trend, -32000, 32000);
}
