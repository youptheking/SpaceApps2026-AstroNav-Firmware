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

// onderste regel, wit op zwart omgedraaid zodat hij opvalt
static void drawAlert(const char *alert) {
  if (!alert) return;
  oled.fillRect(0, 54, 128, 10, SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_BLACK);
  oled.setCursor(1, 55);
  oled.print(alert);
  oled.setTextColor(SSD1306_WHITE);
}

void showPairing(const char *name, uint32_t code, const char *alert) {
  if (!found) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("Verbind met laptop");
  oled.println(name);

  oled.setTextSize(2);
  oled.setCursor(10, 30);
  oled.printf("%03lu %03lu", code / 1000, code % 1000);

  // ook zonder laptop waarschuwen
  drawAlert(alert);
  oled.display();
}

void showStatus(const Telemetry &t, const String &message, const char *alert) {
  if (!found) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);

  if (t.flags & HAS_HEART) oled.printf("HR %3u  SpO2 %u%%\n", t.heart_rate, t.spo2);
  else oled.println("HR --   SpO2 --");

  if (t.flags & HAS_SKIN_TEMP) oled.printf("Temp %.1f C\n", t.skin_temp / 100.0);
  else oled.println("Temp --");

  // mmHg want daar rekent NASA-STD-3001 mee
  if (t.flags & HAS_AIR) oled.printf("CO2 %.2f mmHg\n", t.ppco2_avg / 100.0);
  else oled.println("CO2 --");

  oled.printf("Batt %.2f V\n", t.battery_mv / 1000.0);

  // instructie van het dashboard
  if (message.length()) {
    oled.drawFastHLine(0, 36, 128, SSD1306_WHITE);
    oled.setCursor(0, 40);
    oled.print(message.substring(0, alert ? 21 : 42));
  }

  drawAlert(alert);
  oled.display();
}

void showBreathing(const char *text, float level) {
  if (!found) return;
  oled.clearDisplay();
  oled.setTextSize(2);

  int16_t x, y;
  uint16_t w, h;
  oled.getTextBounds(text, 0, 0, &x, &y, &w, &h);
  oled.setCursor((128 - w) / 2, 0);
  oled.print(text);

  oled.fillCircle(64, 46, 3 + level * 14, SSD1306_WHITE);
  oled.display();
}

}
