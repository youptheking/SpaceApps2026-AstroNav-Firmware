#pragma once
#include <Arduino.h>

// XIAO pin labels (D0 = GPIO1 enz.), volgens docs/pinout.md van de pcb

namespace pins {

constexpr uint8_t ECG_OUT = D0;
constexpr uint8_t ECG_LO_MINUS = D1;
constexpr uint8_t ECG_LO_PLUS = D2;
constexpr uint8_t ECG_SDN = D3;

constexpr uint8_t I2C_SDA = D4;
constexpr uint8_t I2C_SCL = D5;
constexpr uint32_t I2C_FREQ_HZ = 400000;

constexpr uint8_t BUZZER = D7;
constexpr uint8_t BATT = D8;  // 2x 200k deler op BAT_SW+
constexpr uint8_t GSR = D9;   // 220k naar GND, zie analog.cpp

// GSR en AD8232 zijn analoog en niet automatisch te vinden.
// Op true zetten als ze aangesloten zijn, anders worden ze gesimuleerd.
constexpr bool ANALOG_FRONTEND = false;
constexpr float BATT_DIVIDER = 2.0;

}
