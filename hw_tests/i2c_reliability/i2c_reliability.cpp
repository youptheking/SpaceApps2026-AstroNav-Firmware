// Hoe betrouwbaar is de i2c-bus? Per snelheid 50 keer het ID-register van
// elke sensor lezen en tellen hoe vaak het juiste antwoord terugkomt.
// Gaat het bij een lage snelheid beter, dan zijn de flanken te traag
// (te zwakke pull-ups of te veel draad/capaciteit op de bus).
#include <Arduino.h>
#include <Wire.h>

#include "pins.h"

struct Check {
  const char *name;
  uint8_t addr, reg;
  uint8_t ok[4];  // toegestane antwoorden, 0 = einde lijst
};

const Check checks[] = {
  {"MPU6050", 0x68, 0x75, {0x68, 0x70, 0x71, 0x73}},
  {"BMP180", 0x77, 0xD0, {0x55}},
  // als laatste: als hij de bus vasthoudt, zie je de rest in elk geval nog
  {"MAX30102", 0x57, 0xFF, {0x15}},
};

const uint32_t speeds[] = {50000, 100000, 200000, 400000};
const int TRIES = 50;

int readReg(uint8_t addr, uint8_t reg, bool repeatedStart) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(!repeatedStart) != 0) return -1;
  if (Wire.requestFrom(addr, (uint8_t)1) != 1) return -1;
  return Wire.read();
}

bool ping(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
}

void loop() {
  pinMode(pins::I2C_SDA, INPUT);
  pinMode(pins::I2C_SCL, INPUT);
  delay(5);
  Serial.printf("\nSDA %lu mV  SCL %lu mV in rust\n", analogReadMilliVolts(pins::I2C_SDA),
                analogReadMilliVolts(pins::I2C_SCL));

  for (uint32_t hz : speeds) {
    Wire.begin(pins::I2C_SDA, pins::I2C_SCL, hz);
    Wire.setTimeOut(20);
    Serial.printf("%3lu kHz:", hz / 1000);

    for (auto &c : checks) {
      for (bool rs : {true, false}) {
        int good = 0;
        for (int i = 0; i < TRIES; i++) {
          int v = readReg(c.addr, c.reg, rs);
          for (uint8_t want : c.ok) {
            if (want && v == want) good++;
          }
        }
        Serial.printf("  %s%s %2d/%d", c.name, rs ? "" : "(stop)", good, TRIES);
        Serial.flush();
      }
    }

    int scd = 0, oled = 0;
    for (int i = 0; i < TRIES; i++) {
      scd += ping(0x62);
      oled += ping(0x3C);
    }
    Serial.printf("  SCD41-ping %d/%d  OLED-ping %d/%d\n", scd, TRIES, oled, TRIES);
    Wire.end();
  }
  delay(2000);
}
