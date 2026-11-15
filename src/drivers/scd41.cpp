// SCD41: CO2, temperatuur en luchtvochtigheid van de cabine
#include <SensirionI2cScd4x.h>
#include <Wire.h>

#include "drivers.h"

static SensirionI2cScd4x scd;
static uint16_t co2 = 0;
static float temp = 0, humidity = 0;
static uint32_t lastData = 0;
static uint32_t lastPoll = 0;
static bool fresh = false;

static uint32_t pendingPressure = 0;

bool scd41Begin() {
  scd.begin(Wire, SCD41_I2C_ADDR_62);

  // kan nog aan staan van voor een herstart, dan accepteert hij geen instellingen
  scd.stopPeriodicMeasurement();
  delay(500);

  // tot de BMP180 iets meet: 1 atm, de druk van het ISS
  scd.setAmbientPressure(101325);

  // meet zelf elke 5 s, precies wat NASA Tabel 6.1-1 voor CO2 noemt
  return scd.startPeriodicMeasurement() == 0;
}

// ppm hangt af van de luchtdruk, dus de echte cabinedruk doorgeven.
// Mag tijdens het meten, hij neemt hem mee vanaf de volgende meting.
void scd41SetPressure(uint32_t pa) {
  pendingPressure = pa;
}

void scd41Update() {
  if (millis() - lastPoll < 1000) return;
  lastPoll = millis();

  // 30 s niks gemeten (los contact, of het starten mislukte): opnieuw starten
  static uint32_t lastRestart = 0;
  uint32_t since = lastData ? lastData : lastRestart;
  if (millis() - since > 30000 && millis() - lastRestart > 30000) {
    lastRestart = millis();
    scd.stopPeriodicMeasurement();
    delay(500);
    scd.startPeriodicMeasurement();
    Serial.println("scd41: opnieuw gestart");
    return;
  }

  if (pendingPressure) {
    scd.setAmbientPressure(pendingPressure);
    pendingPressure = 0;
  }

  bool ready = false;
  if (scd.getDataReadyStatus(ready) != 0 || !ready) return;

  uint16_t c;
  float t, h;
  if (scd.readMeasurement(c, t, h) == 0 && c > 0) {
    co2 = c;
    temp = t;
    humidity = h;
    lastData = millis();
    fresh = true;
  }
}

bool scd41TakeNew() {
  bool was = fresh;
  fresh = false;
  return was;
}

void scd41Fill(Telemetry &t) {
  // eerste meting duurt 5 s, en na 30 s zonder data vertrouwen we hem niet meer
  if (!lastData || millis() - lastData > 30000) return;
  t.flags |= HAS_AIR;
  t.co2 = co2;
  t.cabin_temp = temp * 100;
  t.humidity = humidity * 100;
}
