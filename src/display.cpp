#include "display.h"

#include <Adafruit_SSD1306.h>
#include <Wire.h>

namespace display {

// 0.96" oled (128x64), 90 graden gedraaid in de behuizing. Het is een
// tweekleurig scherm: de bovenste 16 rijen van het paneel zijn geel, de rest
// blauw. Staand wordt dat een gele strook van 16 pixels aan de zijkant.
// Daar komt het alarm (geel = waarschuwing), de waarden staan in het blauw.
//
// ROTATION 1: gele strook links. Staat alles op zijn kop, zet hem op 3.
static const uint8_t ROTATION = 1;
static const int STRIP = 16;  // breedte van de gele strook
static const int W = 64 - STRIP, H = 128;  // blauwe deel, staand

static Adafruit_SSD1306 oled(128, 64, &Wire, -1);
static bool found_ = false;
static int x0 = 0;  // linkerkant van het blauwe deel

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
    x0 = ROTATION == 1 ? STRIP : 0;
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextWrap(false);
    oled.clearDisplay();
    oled.display();
  }
  return found_;
}

bool found() { return found_; }

// tekst in de gele strook, langs de strook geschreven. Even terug naar de
// liggende stand van het paneel, daar is de strook gewoon rij 0-15.
static void strip(const char *text, bool inverted) {
  oled.setRotation(ROTATION == 1 ? 0 : 2);
  int y = ROTATION == 1 ? 0 : 64 - STRIP;
  if (inverted) oled.fillRect(0, y, 128, STRIP, SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setTextColor(inverted ? SSD1306_BLACK : SSD1306_WHITE);
  oled.setCursor(2, y + 4);
  oled.print(String(text).substring(0, 21));
  oled.setTextColor(SSD1306_WHITE);
  oled.setRotation(ROTATION);
}

// tekst over meerdere regels in het blauwe deel, afgebroken op spaties
static int wrap(const char *text, int y, int maxLines) {
  const int perLine = W / 6;
  int lines = 0;
  const char *p = text;
  oled.setTextSize(1);
  while (*p && lines < maxLines) {
    while (*p == ' ') p++;
    int n = strlen(p);
    if (n > perLine) {
      n = perLine;
      while (n > 0 && p[n] != ' ') n--;
      if (n == 0) n = perLine;  // te lang woord, dan maar hard afbreken
    }
    oled.setCursor(x0, y + lines * 9);
    for (int i = 0; i < n; i++) oled.write(p[i]);
    p += n;
    lines++;
  }
  return lines;
}

static void centered(const char *text, int y, uint8_t size) {
  int16_t bx, by;
  uint16_t w, h;
  oled.setTextSize(size);
  oled.getTextBounds(text, 0, 0, &bx, &by, &w, &h);
  oled.setCursor(x0 + (W - w) / 2, y);
  oled.print(text);
}

// klein label met daaronder een grote waarde, rechts uitgelijnd
static void value(int y, const char *label, const char *v) {
  oled.setTextSize(1);
  oled.setCursor(x0, y);
  oled.print(label);
  oled.setTextSize(2);
  oled.setCursor(x0 + W - strlen(v) * 12, y + 9);
  oled.print(v);
}

void showPairing(const char *name, uint32_t code, const char *alert) {
  if (!found_) return;
  oled.clearDisplay();

  centered("Koppelen", 0, 1);

  // "AstroNav-B8DF" past niet op één regel
  String n = name;
  int dash = n.indexOf('-');
  centered(n.substring(0, dash).c_str(), 14, 1);
  centered(n.substring(dash + 1).c_str(), 24, 1);

  char part[4];
  snprintf(part, sizeof(part), "%03lu", code / 1000);
  centered(part, 42, 2);
  snprintf(part, sizeof(part), "%03lu", code % 1000);
  centered(part, 62, 2);

  wrap("code op de laptop", 92, 3);

  // ook zonder laptop waarschuwen
  if (alert) strip(alert, true);
  else strip("niet verbonden", false);
  oled.display();
}

void showStatus(const Telemetry &t, const String &message, const char *alert) {
  if (!found_) return;
  oled.clearDisplay();
  char v[12];

  if (t.flags & HAS_HEART) snprintf(v, sizeof(v), "%u", t.heart_rate);
  else strcpy(v, "--");
  value(0, "HR", v);

  if (t.flags & HAS_HEART) snprintf(v, sizeof(v), "%u", t.spo2);
  else strcpy(v, "--");
  value(28, "SpO2 %", v);

  // mmHg want daar rekent NASA-STD-3001 mee
  if (t.flags & HAS_AIR) snprintf(v, sizeof(v), "%.2f", t.ppco2_avg / 100.0);
  else strcpy(v, "--");
  value(56, "CO2 mmHg", v);

  // instructie van het dashboard gaat voor temperatuur en accu
  if (message.length()) {
    wrap(message.c_str(), 86, 4);
  } else {
    oled.setTextSize(1);
    oled.setCursor(x0, 86);
    if (t.flags & HAS_SKIN_TEMP) oled.printf("T %.1f C", t.skin_temp / 100.0);
    oled.setCursor(x0, 96);
    oled.printf("B %.2f V", t.battery_mv / 1000.0);
  }

  if (alert) strip(alert, true);
  else strip("verbonden", false);
  oled.display();
}

void showBreathing(const char *text, float level) {
  if (!found_) return;
  oled.clearDisplay();

  strip(text, false);
  oled.fillCircle(x0 + W / 2, H / 2, 3 + level * 20, SSD1306_WHITE);
  oled.display();
}

}
