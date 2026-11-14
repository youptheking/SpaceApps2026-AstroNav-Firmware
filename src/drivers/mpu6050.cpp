// MPU-6050: beweging, activiteit, tremor, schok en dubbeltik.
// Rechtstreeks via de registers: het protocol wil de ruwe tellingen, en zo
// werken ook de goedkope klonen (MPU-6500) die een ander chip-ID geven.
#include <Wire.h>

#include "../motion.h"
#include "../packets.h"
#include "../sampler.h"
#include "astronav_protocol.h"
#include "drivers.h"

using namespace astronav;

const uint8_t ADDR = 0x68;
const float SAMPLE_RATE = 50;     // tot 25 Hz te zien, ruim boven de tremorband
const float LSB_PER_G = 8192;     // ±4 g
const float ARTIFACT_MG = 60;     // meer beweging = PPG niet te vertrouwen
const float REST_MG = 25, LIGHT_MG = 150;  // grenzen activiteit 0/1/2, eigen keuze

static Motion motion(SAMPLE_RATE);
static ImuPacket packet;
static int inPacket = 0;
static volatile uint32_t samples = 0;

static void write8(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

bool mpu6050Begin() {
  Wire.beginTransmission(ADDR);
  Wire.write(0x75);  // WHO_AM_I
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(ADDR, (uint8_t)1) != 1) return false;
  uint8_t id = Wire.read();
  // 0x68 = MPU-6050, 0x70 = MPU-6500 (veel klonen), 0x71/0x73 = MPU-9250/9255
  if (id != 0x68 && id != 0x70 && id != 0x71 && id != 0x73) return false;

  // geen reset (0x80): daarna gaf deze module geen antwoord meer
  write8(0x6B, 0x01);  // wakker, klok van de gyro-PLL
  write8(0x1A, 0x04);  // interne filter op ~21 Hz, anders vouwt alles boven 25 Hz terug
  write8(0x19, 19);    // 1 kHz / 20 = 50 Hz
  write8(0x1B, 0x08);  // gyro ±500 °/s
  write8(0x1C, 0x08);  // accel ±4 g
  return true;
}

void mpu6050Sample(uint32_t now) {
  Wire.beginTransmission(ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(ADDR, (uint8_t)14) != 14) return;

  int16_t v[7];  // ax ay az temp gx gy gz, big-endian
  for (auto &x : v) x = Wire.read() << 8 | Wire.read();

  if (inPacket == 0) packets::header(packet.h, PKT_IMU, now);
  packet.s[inPacket] = {v[0], v[1], v[2], v[4], v[5], v[6]};
  if (++inPacket == 5) {
    if (sampler::rawStreams()) sampler::push(&packet, sizeof(packet));
    inPacket = 0;
  }

  const float toMg = 1000 / LSB_PER_G;
  motion.add(v[0] * toMg, v[1] * toMg, v[2] * toMg);
  samples++;
}

bool mpu6050TakeDoubleTap() {
  return motion.takeDoubleTap();
}

void mpu6050Fill(Telemetry &t) {
  if (samples < SAMPLE_RATE * 2) return;  // filters eerst even laten inlopen
  t.flags |= HAS_IMU;

  float act = motion.activityMg();
  t.activity_mg = min(act, 65535.0f);
  t.activity = act < REST_MG ? 0 : act < LIGHT_MG ? 1 : 2;
  t.tremor = min(motion.tremorMg(), 255.0f);
  t.tremor_hz_c10 = motion.tremorHz() * 10;

  if (act > ARTIFACT_MG) t.vitals_flags |= FLAG_MOTION_ARTIFACT;
  t.shock_mg = motion.shockMg();
  if (t.shock_mg) t.vitals_flags |= FLAG_SHOCK;
}
