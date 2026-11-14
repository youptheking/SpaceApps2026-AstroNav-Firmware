#include "sensors.h"

#include <Wire.h>

#include "drivers/drivers.h"

namespace sensors {

static bool ok[16];
static uint8_t okMask = 0;
static uint8_t groupMask = 0;

static bool ping(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

void begin() {
  for (size_t i = 0; i < driverCount; i++) {
    const Driver &d = drivers[i];
    // eerst pingen, dan hangt een begin() niet op een sensor die er niet is
    ok[i] = (d.addr == 0 || ping(d.addr)) && d.begin();
    if (ok[i]) {
      okMask |= d.okBit;
      groupMask |= d.groups;
    }
  }
}

void update() {
  for (size_t i = 0; i < driverCount; i++) {
    if (ok[i] && drivers[i].update) drivers[i].update();
  }
}

void fill(Telemetry &t) {
  for (size_t i = 0; i < driverCount; i++) {
    if (ok[i]) drivers[i].fill(t);
  }
}

bool has(uint8_t okBit) { return okMask & okBit; }
uint8_t okBits() { return okMask; }
uint8_t groups() { return groupMask; }

void print() {
  for (size_t i = 0; i < driverCount; i++) {
    const Driver &d = drivers[i];
    if (d.addr) Serial.printf("%-9s 0x%02X  %s\n", d.name, d.addr, ok[i] ? "ok" : "niet gevonden");
    else Serial.printf("%-9s analog %s\n", d.name, ok[i] ? "ok" : "uit");
  }
}

}
