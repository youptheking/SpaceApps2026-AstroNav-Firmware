// MCP9808: huidtemperatuur, onderkant van het horloge
#include <Adafruit_MCP9808.h>
#include <Wire.h>

#include "drivers.h"

static Adafruit_MCP9808 sensor;
static float temp = NAN;
static uint32_t lastRead = 0;

bool mcp9808Begin() {
  if (!sensor.begin(0x18, &Wire)) return false;
  sensor.setResolution(3);  // 0,0625 °C, duurt 250 ms per meting
  sensor.wake();
  return true;
}

void mcp9808Update() {
  if (millis() - lastRead < 1000) return;
  lastRead = millis();

  float t = sensor.readTempC();
  // buiten dit bereik zit hij niet tegen een mens aan
  temp = (t > 20 && t < 45) ? t : NAN;
}

void mcp9808Fill(Telemetry &t) {
  if (isnan(temp)) return;
  t.flags |= HAS_SKIN_TEMP;
  t.skin_temp = temp * 100;
}
