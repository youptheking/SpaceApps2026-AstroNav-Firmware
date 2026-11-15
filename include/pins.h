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
constexpr uint32_t I2C_FREQ_HZ = 100000;  // 400k gaf leesfouten op de gefreesde pcb

constexpr uint8_t BUZZER = D7;
constexpr uint8_t BATT = D8;  // 2x 200k deler op BAT_SW+
constexpr uint8_t GSR = D9;   // 220k naar GND, zie drivers/gsr.cpp

// GSR en AD8232 zijn analoog en niet automatisch te vinden. Ze zitten op de
// pcb, dus aan. Op false zetten om zonder te testen.
constexpr bool ANALOG_FRONTEND = true;
constexpr float BATT_DIVIDER = 2.0;

// Op het prototype zijn de accuconnector en de schakelaar kapot, dus het
// draait altijd op usb. Dan hangt D8 los en zou een zwevende waarde een vals
// accupercentage of "Battery low" geven. Weer op true als er een accu aan zit.
constexpr bool HAS_BATTERY = false;

// Demomodus: wat er op het prototype niet is (licht, accu, of een sensor die
// wegvalt) krijgt nepwaarden, en het SIM-pakket vertelt het dashboard welke.
// Echte sensoren gaan altijd voor. Uit voor een eerlijke meting zonder vulling.
constexpr bool DEMO_MODE = true;

}
