// i2c scan, laat zien welke sensoren er hangen
#include <Arduino.h>
#include <Wire.h>

#include "pins.h"

struct Device {
  uint8_t addr;
  const char *name;
};

Device devices[] = {
  {0x18, "MCP9808"},
  {0x23, "BH1750"},
  {0x3C, "OLED"},
  {0x57, "MAX30102"},
  {0x62, "SCD40"},
  {0x68, "MPU6050"},
};

bool ping(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Wire.begin(pins::I2C_SDA, pins::I2C_SCL, pins::I2C_FREQ_HZ);
}

void loop() {
  Serial.println("\nscan:");
  for (auto &d : devices) {
    Serial.printf("0x%02X %-9s %s\n", d.addr, d.name, ping(d.addr) ? "ok" : "-");
  }

  for (uint8_t addr = 1; addr < 127; addr++) {
    bool known = false;
    for (auto &d : devices) {
      if (d.addr == addr) known = true;
    }
    if (!known && ping(addr)) {
      Serial.printf("0x%02X ??\n", addr);
    }
  }

  delay(3000);
}
