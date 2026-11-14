#include "display.h"

#include <Adafruit_SSD1306.h>
#include <Wire.h>

namespace display {

// 0.96" oled (128x64), 90 graden gedraaid in de behuizing. Staand is het
// scherm dus 64 breed en 128 hoog: 10 tekens per regel bij tekstgrootte 1.
// Staat alles op zijn kop, zet ROTATION op 3.
static const uint8_t ROTATION = 1;
static Adafruit_SSD1306 oled(128, 64, &Wire, -1);
static bool found_ = false;
static int W = 64, H = 128;

static bool ping(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

bool begin() {
  // begin() geeft ook true zonder scherm, dus eerst zelf kijken. Het scherm
  // is vlak na het aanzetten soms nog niet wakker, dus een paar keer proberen.
  // De meeste zitten op 0x3C, sommige 0.96" op 0x3D.
  uint8_t addr = 0;
  for (int i = 0; i < 5 && !addr; i++) {
    if (i) delay(50);
    if (ping(0x3C)) addr = 0x3C;
    else if (ping(0x3D)) addr = 0x3D;
  }
  if (!addr) return false;

  // false: Wire is al gestart in main, niet opnieuw laten doen
  found_ = oled.begin(SSD1306_SWITCHCAPVCC, addr, true, false);
  if (found_) {
    oled.setRotation(ROTATION);
    W = oled.width();
    H = oled.height();
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextWrap(false);
    oled.clearDisplay();
    oled.display();
  }
  return found_;
}

bool found() { return found_; }

// tekst over meerdere regels, afgebroken op spaties. Geeft het aantal regels.
static int wrap(const char *text, int y, int maxLines, bool draw = true) {
  const int perLine = W / 6;
  int lines = 0;
  const char *p = text;
  while (*p && lines < maxLines) {
    while (*p == ' ') p++;
    int len = strlen(p);
    int n = len;
    if (n > perLine) {
      n = perLine;
      while (n > 0 && p[n] != ' ') n--;
      if (n == 0) n = perLine;  // te lang woord, dan maar hard afbreken
    }
    if (draw) {
      oled.setCursor(0, y + lines * 9);
      for (int i = 0; i < n; i++) oled.write(p[i]);
    }
    p += n;
    lines++;
  }
  return lines;
}

// onderin, omgedraaid zodat hij opvalt. Geeft de bovenkant terug.
static int drawAlert(const char *alert) {
  if (!alert) return H;
  oled.setTextSize(1);
  int lines = wrap(alert, 0, 3, false);
  int top = H - lines * 9 - 2;
  oled.fillRect(0, top, W, H - top, SSD1306_WHITE);
  oled.setTextColor(SSD1306_BLACK);
  wrap(alert, top + 2, 3);
  oled.setTextColor(SSD1306_WHITE);
  return top;
}

static void centered(const char *text, int y, uint8_t size) {
  int16_t x1, y1;
  uint16_t w, h;
  oled.setTextSize(size);
  oled.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  oled.setCursor((W - w) / 2, y);
  oled.print(text);
}

void showPairing(const char *name, uint32_t code, const char *alert) {
  if (!found_) return;
  oled.clearDisplay();

  centered("Koppelen", 0, 1);
  oled.drawFastHLine(0, 10, W, SSD1306_WHITE);

  // "AstroNav-B8DF" past niet op één regel
  String n = name;
  int dash = n.indexOf('-');
  centered(n.substring(0, dash).c_str(), 16, 1);
  centered(n.substring(dash + 1).c_str(), 26, 1);

  // code groot, in twee stukken van drie
  char part[4];
  snprintf(part, sizeof(part), "%03lu", code / 1000);
  centered(part, 44, 3);
  snprintf(part, sizeof(part), "%03lu", code % 1000);
  centered(part, 70, 3);

  // ook zonder laptop waarschuwen
  if (alert) drawAlert(alert);
  else {
    oled.setTextSize(1);
    wrap("code op de laptop", 104, 2);
  }
  oled.display();
}

// klein label met daarnaast een grote waarde
static void row(int y, const char *label, const char *value) {
  oled.setTextSize(1);
  oled.setCursor(0, y + 4);
  oled.print(label);
  oled.setTextSize(2);
  oled.setCursor(W - strlen(value) * 12, y);
  oled.print(value);
}

void showStatus(const Telemetry &t, const String &message, const char *alert) {
  if (!found_) return;
  oled.clearDisplay();
  char v[12];

  if (t.flags & HAS_HEART) snprintf(v, sizeof(v), "%u", t.heart_rate);
  else strcpy(v, "--");
  row(0, "HR", v);

  if (t.flags & HAS_HEART) snprintf(v, sizeof(v), "%u", t.spo2);
  else strcpy(v, "--");
  row(18, "O2%", v);

  // mmHg want daar rekent NASA-STD-3001 mee
  oled.setTextSize(1);
  oled.setCursor(0, 38);
  oled.print("CO2 mmHg");
  if (t.flags & HAS_AIR) snprintf(v, sizeof(v), "%.2f", t.ppco2_avg / 100.0);
  else strcpy(v, "--");
  oled.setTextSize(2);
  oled.setCursor(W - strlen(v) * 12, 48);
  oled.print(v);

  oled.setTextSize(1);
  oled.setCursor(0, 68);
  if (t.flags & HAS_SKIN_TEMP) oled.printf("T %.1f C", t.skin_temp / 100.0);
  oled.setCursor(0, 77);
  oled.printf("B %.2f V", t.battery_mv / 1000.0);

  int bottom = drawAlert(alert);

  // instructie van het dashboard in de ruimte tussen de waarden en het alarm
  if (message.length()) {
    oled.drawFastHLine(0, 88, W, SSD1306_WHITE);
    oled.setTextSize(1);
    wrap(message.c_str(), 92, (bottom - 92) / 9);
  }
  oled.display();
}

void showBreathing(const char *text, float level) {
  if (!found_) return;
  oled.clearDisplay();

  centered(text, 8, 1);
  oled.fillCircle(W / 2, 74, 4 + level * 26, SSD1306_WHITE);
  oled.display();
}

}
