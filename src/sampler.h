#pragma once
#include <Arduino.h>

// Eigen task voor de snelle sensoren, op een vaste tik van 4 ms (250 Hz).
// ECG, PPG, IMU en GSR krijgen zo allemaal hun tijdstempel van dezelfde
// klok, nodig voor RR-intervallen en later PTT (docs/ble-data.md).
namespace sampler {

void begin();

// pakket voor de laptop klaarzetten, vanuit de sample-task
void push(const void *packet, size_t len);

// volgende pakket ophalen in loop(), false als er niks is
bool pop(uint8_t *buf, size_t &len);

// ruwe streams (ECG/PPG/IMU/GSR) aan of uit, control 0x02
void setRawStreams(bool on);
bool rawStreams();

uint32_t dropped();  // pakketten die niet meer in de rij pasten

}
