// MPU-6050: beweging, activiteit en tremor
#include <Adafruit_MPU6050.h>
#include <Wire.h>

#include "../motion.h"
#include "drivers.h"

const float SAMPLE_RATE = 50;  // tot 25 Hz te zien, ruim boven de tremorband
const uint32_t PERIOD_US = 1000000 / SAMPLE_RATE;

static Adafruit_MPU6050 mpu;
static Motion motion(SAMPLE_RATE);
static uint32_t nextSample = 0;
static uint32_t samples = 0;

bool mpu6050Begin() {
  if (!mpu.begin(0x68, &Wire)) return false;
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  // interne filter op 21 Hz, anders vouwt alles boven 25 Hz terug in de band
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  nextSample = micros();
  return true;
}

void mpu6050Update() {
  if ((int32_t)(micros() - nextSample) < 0) return;
  nextSample += PERIOD_US;
  // meer dan 100 ms achter (bijv. scherm of flash), dan niet inhalen maar opnieuw beginnen
  if ((int32_t)(micros() - nextSample) > 100000) nextSample = micros() + PERIOD_US;

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  const float toMg = 1000 / SENSORS_GRAVITY_STANDARD;
  motion.add(a.acceleration.x * toMg, a.acceleration.y * toMg, a.acceleration.z * toMg);
  samples++;
}

void mpu6050Fill(Telemetry &t) {
  if (samples < SAMPLE_RATE * 2) return;  // filters eerst even laten inlopen
  t.flags |= HAS_IMU;
  for (int i = 0; i < 3; i++) t.accel[i] = motion.mean(i);
  t.activity = motion.activity();
  t.tremor = motion.tremor();
}
