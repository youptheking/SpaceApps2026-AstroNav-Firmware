#pragma once
#include <Arduino.h>

#include "telemetry.h"

// Alle sensoren. Bij het opstarten wordt gekeken welke er zijn; wat ontbreekt
// levert niks (of nepwaarden met `sim on`, zie sim.cpp).
namespace sensors {

const uint8_t ALL = HAS_HEART | HAS_SKIN_TEMP | HAS_AIR | HAS_PRESSURE | HAS_IMU | HAS_GSR | ECG_LEADS_OK;

void begin();
void update();            // in loop(), voor de langzame sensoren
void fill(Telemetry &t);  // laatste meetwaarden in de stand zetten

bool has(uint8_t okBit);  // OK_* uit astronav_protocol.h
uint8_t okBits();         // voor STATUS.sensors_ok, zonder het OLED
uint8_t groups();         // SensorFlag-bits van de sensoren die er zijn

void print();  // usb: overzicht

}
