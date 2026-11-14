#include "display.h"

#include <Adafruit_SSD1306.h>
#include <Wire.h>

namespace display {

// 0.91" oled op de pcb: 128x32, dus 4 regels tekst van 8 pixels
static const int W = 128, H = 32;
static Adafruit_SSD1306 oled(W, H, &Wire, -1);
static bool found = false;

bool begin() {
  // begin() geeft ook true zonder scherm, dus eerst zelf kijken
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() != 0) return false;

  // false: Wire is al gestart in main, niet opnieuw laten doen
  found = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C, true, false);
  if (found) {
    oled.setTextColor(SSD1306_WHITE);
    oled.clearDisplay();
    oled.display();
  }
  return found;
}

// onderste regel, omgedraaid zodat hij opvalt
static void drawAlert(const char *alert) {
  if (!alert) return;
  oled.fillRect(0, 24, W, 8, SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_BLACK);
  oled.setCursor(1, 24);
  oled.print(alert);
  oled.setTextColor(SSD1306_WHITE);
}

void showPairing(const char *name, uint32_t code, const char *alert) {
  if (!found) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(name);

  oled.setTextSize(2);
  oled.setCursor(0, 8);
  oled.printf("%03lu %03lu", code / 1000, code % 1000);

  // ook zonder laptop waarschuwen
  if (alert) drawAlert(alert);
  else {
    oled.setTextSize(1);
    oled.setCursor(0, 24);
    oled.print("koppelcode laptop");
  }
  oled.display();
}

void showStatus(const Telemetry &t, const String &message, const char *alert) {
  if (!found) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);

  if (t.flags & HAS_HEART) oled.printf("HR %3u  SpO2 %u%%\n", t.heart_rate, t.spo2);
  else oled.println("HR --   SpO2 --");

  // mmHg want daar rekent NASA-STD-3001 mee
  if (t.flags & HAS_AIR) oled.printf("CO2 %.2f mmHg", t.ppco2_avg / 100.0);
  else oled.print("CO2 --");
  oled.setCursor(0, 16);

  // instructie van het dashboard gaat voor de accu, de laatste regel is
  // voor het alarm of het vervolg van het bericht
  if (message.length()) {
    oled.print(message.substring(0, 21));
    if (!alert && message.length() > 21) {
      oled.setCursor(0, 24);
      oled.print(message.substring(21, 42));
    }
  } else {
    if (t.flags & HAS_SKIN_TEMP) oled.printf("%.1f C  ", t.skin_temp / 100.0);
    oled.printf("Batt %.2f V", t.battery_mv / 1000.0);
  }

  drawAlert(alert);
  oled.display();
}

void showBreathing(const char *text, float level) {
  if (!found) return;
  oled.clearDisplay();

  // cirkel rechts, tekst links ernaast. "Vasthouden" past niet groot
  int16_t x, y;
  uint16_t w, h;
  oled.setTextSize(2);
  oled.getTextBounds(text, 0, 0, &x, &y, &w, &h);
  if (w > 96) {
    oled.setTextSize(1);
    oled.getTextBounds(text, 0, 0, &x, &y, &w, &h);
  }
  oled.setCursor(0, (H - h) / 2);
  oled.print(text);

  oled.fillCircle(113, 16, 2 + level * 12, SSD1306_WHITE);
  oled.display();
}

}
