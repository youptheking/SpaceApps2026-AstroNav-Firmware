#pragma once
#include <Arduino.h>

#include "../telemetry.h"

// Eén driver per sensor. begin() geeft false als de sensor er niet is.
// update() draait in loop(), voor de langzame sensoren. De snelle sensoren
// hebben daarnaast een *Sample() die de sample-task aanroept (sampler.cpp).
struct Driver {
  const char *name;
  uint8_t addr;    // i2c-adres, 0 = analoog
  uint8_t okBit;   // bit in STATUS.sensors_ok
  uint8_t groups;  // bits uit SensorFlag die deze sensor levert
  bool (*begin)();
  void (*update)();  // mag nullptr zijn
  void (*fill)(Telemetry &t);
};

extern const Driver drivers[];
extern const size_t driverCount;

// SCD41: CO2, cabinetemperatuur, luchtvochtigheid
bool scd41Begin();
void scd41Update();
void scd41Fill(Telemetry &t);
bool scd41TakeNew();  // true als er sinds de vorige keer een nieuwe meting is
void scd41SetPressure(uint32_t pa);

// BMP180: cabinedruk
bool bmp180Begin();
void bmp180Update();
void bmp180Fill(Telemetry &t);
float bmp180Temperature();  // °C, om de meting na te kijken

// MAX30102: PPG, hartslag, SpO2, chiptemperatuur
bool max30102Begin();
void max30102Sample(uint32_t now);
void max30102Update();  // het rekenwerk, in loop()
void max30102Fill(Telemetry &t);

// MPU-6050: beweging
bool mpu6050Begin();
void mpu6050Sample(uint32_t now);
void mpu6050Fill(Telemetry &t);
bool mpu6050TakeDoubleTap();

// AD8232 (ECG) en de GSR-vlakken, allebei analoog
bool ecgBegin();
void ecgSample(uint32_t now);
void rrSend(uint32_t now);
void ecgFill(Telemetry &t);
bool gsrBegin();
void gsrSample(uint32_t now);
void gsrFill(Telemetry &t);
