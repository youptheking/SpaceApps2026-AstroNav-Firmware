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

bool max30102Begin();
void max30102Update();
void max30102Fill(Telemetry &t);

bool mcp9808Begin();
void mcp9808Update();
void mcp9808Fill(Telemetry &t);

bool bh1750Begin();
void bh1750Update();
void bh1750Fill(Telemetry &t);

bool mpu6050Begin();
void mpu6050Update();
void mpu6050Fill(Telemetry &t);
