#pragma once
#include <Arduino.h>

#include "../telemetry.h"

// Eén driver per sensor. begin() geeft false als de sensor er niet is.
struct Driver {
  const char *name;
  uint8_t addr;   // i2c-adres, 0 = analoog
  uint8_t group;  // bits uit SensorFlag
  bool (*begin)();
  void (*update)();  // mag nullptr zijn
  void (*fill)(Telemetry &t);
};

extern const Driver drivers[];
extern const size_t driverCount;

bool scd40Begin();
void scd40Update();
void scd40Fill(Telemetry &t);
