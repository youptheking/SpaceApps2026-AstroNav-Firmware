// i2c scan, laat zien welke sensoren er hangen
//
// Doet i2c zelf (bit-bang) in plaats van met Wire. Houdt een module de bus
// laag, dan blijft Wire hangen en zie je niks. Zo zie je juist welke lijn.
#include <Arduino.h>
#include <BLEDevice.h>
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

// SCL loslaten en meten hoe lang iemand hem nog laag houdt (clock stretching).
// Langste tijd wordt bijgehouden, -1 = na 50 ms nog steeds laag.
static int32_t longestHold = 0;
bool sclWait() {
  release(BUS_SCL);
  uint32_t start = micros();
  while (!digitalRead(BUS_SCL)) {
    if (micros() - start > 50000) {
      longestHold = -1;
      return false;
    }
  }
  int32_t held = micros() - start;
  if (longestHold >= 0 && held > longestHold) longestHold = held;
  return true;
}

// 1 = ack, 0 = nack, -1 = SCL zit vast
int sendByte(uint8_t b) {
  for (int bit = 7; bit >= 0; bit--) {
    if (b & (1 << bit)) release(BUS_SDA);
    else low(BUS_SDA);
    delayMicroseconds(HALF);
    if (!sclWait()) return -1;
    delayMicroseconds(HALF);
    low(BUS_SCL);
  }
  release(BUS_SDA);
  delayMicroseconds(HALF);
  if (!sclWait()) return -1;
  bool ack = !digitalRead(BUS_SDA);
  delayMicroseconds(HALF);
  low(BUS_SCL);
  return ack;
}

// register lezen in software, en zeggen in welke fase het misgaat
const char *readRegBB(uint8_t addr, uint8_t reg, int &value) {
  value = -1;
  longestHold = 0;
  release(BUS_SDA);
  if (!sclWait()) return "SCL laag voor de start";
  low(BUS_SDA);
  delayMicroseconds(HALF);
  low(BUS_SCL);

  const char *fail = nullptr;
  int r = sendByte(addr << 1);
  if (r < 0) fail = "SCL vast na het adres";
  else if (r == 0) fail = "geen ack op het adres";
  if (!fail) {
    r = sendByte(reg);
    if (r < 0) fail = "SCL vast na het registernummer";
    else if (r == 0) fail = "geen ack op het registernummer";
  }
  if (!fail) {
    // herhaalde start, dan lezen
    release(BUS_SDA);
    delayMicroseconds(HALF);
    if (!sclWait()) fail = "SCL vast bij herhaalde start";
    else {
      low(BUS_SDA);
      delayMicroseconds(HALF);
      low(BUS_SCL);
      r = sendByte(addr << 1 | 1);
      if (r < 0) fail = "SCL vast na het leesadres";
      else if (r == 0) fail = "geen ack op het leesadres";
    }
  }
  if (!fail) {
    release(BUS_SDA);
    int v = 0;
    for (int bit = 0; bit < 8 && !fail; bit++) {
      delayMicroseconds(HALF);
      if (!sclWait()) fail = "SCL vast tijdens de data";
      v = v << 1 | digitalRead(BUS_SDA);
      delayMicroseconds(HALF);
      low(BUS_SCL);
    }
    // nack: we willen maar één byte
    release(BUS_SDA);
    delayMicroseconds(HALF);
    sclWait();
    delayMicroseconds(HALF);
    low(BUS_SCL);
    if (!fail) value = v;
  }
  stop();
  return fail ? fail : "ok";
}

void readTest() {
  struct { const char *name; uint8_t addr, reg; } regs[] = {
    {"MPU6050 WHO_AM_I", 0x68, 0x75},
    {"BMP180 ID", 0x77, 0xD0},
    {"MAX30102 ID", 0x57, 0xFF},
    {"OLED status", 0x3C, 0x00},
  };
  for (auto &r : regs) {
    int v;
    const char *result = readRegBB(r.addr, r.reg, v);
    Serial.printf("  %-17s %-32s waarde %3d  SCL max %ld us laag gehouden\n", r.name, result, v, longestHold);
  }
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

int readReg(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return -1;
  if (Wire.requestFrom(addr, (uint8_t)1) != 1) return -1;
  return Wire.read();
}

// MPU-6050 lezen, wakker maken en nog eens lezen. Valt hij weg als hij
// wakker is, dan krijgt hij te weinig stroom (slecht soldeerpunt?)
void mpuTest() {
  Wire.begin(BUS_SDA, BUS_SCL, 100000);
  Serial.printf("MPU slapend:  WHO_AM_I %d  PWR 0x%02X\n", readReg(0x68, 0x75), readReg(0x68, 0x6B));
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0x01);
  Serial.printf("MPU wakker maken: fout %u\n", Wire.endTransmission());
  for (int ms : {10, 100, 1000}) {
    delay(ms);
    Serial.printf("MPU wakker na %4d ms: WHO_AM_I %d  PWR 0x%02X\n", ms, readReg(0x68, 0x75), readReg(0x68, 0x6B));
  }
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0x40);  // weer slapen
  Wire.endTransmission();
  Wire.end();
}

// na 3 rondes bluetooth aan: stoort de radio de bus?
void maybeStartBle() {
  static int rounds = 0;
  if (++rounds != 4) return;
  BLEDevice::init("AstroNav-test");
  BLEDevice::getAdvertising()->start();
  Serial.println("\n*** bluetooth staat nu aan ***");
}

void loop() {
  maybeStartBle();
  Serial.println("\nscan:");
  printLines();
  Serial.println("register lezen in software:");
  readTest();
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
    for (uint8_t addr = 1; addr < 127; addr++) {
      Wire.beginTransmission(addr);
      if (Wire.endTransmission() == 0) Serial.printf(" 0x%02X", addr);
    }
    Serial.println();
    Wire.end();
  }

  delay(3000);
}
