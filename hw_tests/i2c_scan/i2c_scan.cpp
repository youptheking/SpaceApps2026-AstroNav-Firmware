// i2c scan, laat zien welke sensoren er hangen
//
// Doet i2c zelf (bit-bang) in plaats van met Wire. Houdt een module de bus
// laag, dan blijft Wire hangen en zie je niks. Zo zie je juist welke lijn.
#include <Arduino.h>
#include <Wire.h>

#include "pins.h"

struct Device {
  uint8_t addr;
  const char *name;
};

Device devices[] = {
  {0x18, "MCP9808"},
  {0x3C, "OLED"},
  {0x57, "MAX30102"},
  {0x62, "SCD41"},
  {0x68, "MPU6050"},
  {0x77, "BMP180"},
};

const uint8_t BUS_SDA = pins::I2C_SDA, BUS_SCL = pins::I2C_SCL;
const int HALF = 5;  // µs, ~100 kHz

// open-drain: laag trekken of loslaten, de pull-ups maken hem hoog
void low(uint8_t pin) { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); }
void release(uint8_t pin) { pinMode(pin, INPUT); }

// laat SCL los en wacht als een module hem vasthoudt (clock stretching)
bool sclHigh() {
  release(BUS_SCL);
  uint32_t start = micros();
  while (!digitalRead(BUS_SCL)) {
    if (micros() - start > 2000) return false;
  }
  return true;
}

void stop() {
  low(BUS_SDA);
  delayMicroseconds(HALF);
  sclHigh();
  delayMicroseconds(HALF);
  release(BUS_SDA);
  delayMicroseconds(HALF);
}

// 9 klokpulsen: een module die midden in een byte bleef hangen laat SDA dan los
void recover() {
  release(BUS_SDA);
  for (int i = 0; i < 9 && !digitalRead(BUS_SDA); i++) {
    low(BUS_SCL);
    delayMicroseconds(HALF);
    sclHigh();
    delayMicroseconds(HALF);
  }
  stop();
}

// -1 = bus zit vast, 0 = geen antwoord, 1 = ack
int ping(uint8_t addr) {
  if (!digitalRead(BUS_SDA) || !digitalRead(BUS_SCL)) return -1;

  low(BUS_SDA);  // start
  delayMicroseconds(HALF);
  low(BUS_SCL);

  uint8_t byte = addr << 1;  // schrijven
  for (int bit = 7; bit >= 0; bit--) {
    if (byte & (1 << bit)) release(BUS_SDA);
    else low(BUS_SDA);
    delayMicroseconds(HALF);
    if (!sclHigh()) return -1;
    delayMicroseconds(HALF);
    low(BUS_SCL);
  }

  release(BUS_SDA);
  delayMicroseconds(HALF);
  if (!sclHigh()) return -1;
  bool ack = !digitalRead(BUS_SDA);
  delayMicroseconds(HALF);
  low(BUS_SCL);
  delayMicroseconds(HALF);
  stop();
  return ack;
}

void printLines() {
  release(BUS_SDA);
  release(BUS_SCL);
  delay(1);
  // ~3300 mV is goed, ~1800 mV = pull-ups van de MAX30102 op 1,8 V
  Serial.printf("SDA %lu mV  SCL %lu mV\n", analogReadMilliVolts(BUS_SDA), analogReadMilliVolts(BUS_SCL));
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("i2c scan");
}

void loop() {
  Serial.println("\nscan:");
  printLines();
  release(BUS_SDA);
  release(BUS_SCL);
  if (!digitalRead(BUS_SDA)) {
    Serial.println("SDA wordt laag gehouden, probeer te herstellen");
    recover();
  }
  if (!digitalRead(BUS_SCL)) Serial.println("SCL wordt laag gehouden door een module");

  for (auto &d : devices) {
    int r = ping(d.addr);
    Serial.printf("0x%02X %-9s %s\n", d.addr, d.name, r > 0 ? "ok" : r < 0 ? "BUS VAST" : "-");
  }

  for (uint8_t addr = 1; addr < 127; addr++) {
    bool known = false;
    for (auto &d : devices) {
      if (d.addr == addr) known = true;
    }
    if (!known && ping(addr) > 0) Serial.printf("0x%02X ??\n", addr);
  }

  // dezelfde adressen via de hardware-i2c (Wire), op een paar snelheden
  for (uint32_t hz : {100000, 50000, 20000}) {
    Wire.begin(BUS_SDA, BUS_SCL, hz);
    Wire.setTimeOut(50);
    Serial.printf("Wire %3lu kHz:", hz / 1000);
    for (auto &d : devices) {
      Wire.beginTransmission(d.addr);
      if (Wire.endTransmission() == 0) Serial.printf(" %s", d.name);
    }
    Serial.println();
    Wire.end();
  }

  delay(3000);
}
