#include "display.h"

#include <Adafruit_SSD1306.h>
#include <Wire.h>

namespace display {

static Adafruit_SSD1306 oled(128, 64, &Wire, -1);
static bool found = false;

bool begin() {
  // begin() geeft ook true zonder scherm, dus eerst zelf kijken
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() != 0) return false;

  found = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (found) {
    oled.setTextColor(SSD1306_WHITE);
    oled.clearDisplay();
    oled.display();
  }
  return found;
}

void showPairing(const char *name, uint32_t code) {
  if (!found) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("Verbind met laptop");
  oled.println(name);

  oled.setTextSize(2);
  oled.setCursor(10, 30);
  oled.printf("%03lu %03lu", code / 1000, code % 1000);
  oled.display();
}

void showStatus(const Telemetry &t, const String &message) {
  if (!found) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);

  if (t.flags & HAS_HEART) oled.printf("HR %3u  SpO2 %u%%\n", t.heart_rate, t.spo2);
  else oled.println("HR --   SpO2 --");

  if (t.flags & HAS_SKIN_TEMP) oled.printf("Temp %.1f C\n", t.skin_temp / 100.0);
  else oled.println("Temp --");

  if (t.flags & HAS_AIR) oled.printf("CO2 %u ppm\n", t.co2);
  else oled.println("CO2 --");

  oled.printf("Batt %.2f V\n", t.battery_mv / 1000.0);

  // instructie van het dashboard onderaan
  if (message.length()) {
    oled.drawFastHLine(0, 38, 128, SSD1306_WHITE);
    oled.setCursor(0, 42);
    oled.print(message);
  }
  oled.display();
}

}
