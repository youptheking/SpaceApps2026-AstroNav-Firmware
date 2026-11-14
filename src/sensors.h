#pragma once
#include <Arduino.h>

#include "telemetry.h"

// Echte sensoren. Wat bij het opstarten niet gevonden wordt, vult sim.cpp aan,
// zodat je de breakouts één voor één kunt aansluiten.
namespace sensors {

const uint8_t ALL = HAS_HEART | HAS_SKIN_TEMP | HAS_AIR | HAS_IMU | HAS_GSR;

void begin();
void update();            // zo vaak mogelijk, voor de snelle sensoren
void fill(Telemetry &t);  // laatste echte meetwaarden in het pakket

// sensorgroepen (bits uit SensorFlag) waarvan de hardware er is
uint8_t found();

void print();  // usb: overzicht

}
