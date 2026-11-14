// BH1750: omgevingslicht (totaal, geen blauw licht), bovenkant
#include <BH1750.h>
#include <Wire.h>

#include "drivers.h"

static BH1750 sensor(0x23);
static float lux = -1;
static uint32_t lastRead = 0;

bool bh1750Begin() {
  return sensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
}

void bh1750Update() {
  if (millis() - lastRead < 1000) return;
  lastRead = millis();
  if (sensor.measurementReady()) lux = sensor.readLightLevel();  // < 0 = fout
}

void bh1750Fill(Telemetry &t) {
  if (lux < 0) return;
  t.flags |= HAS_LIGHT;
  t.lux = min(lux, 65535.0f);
}
