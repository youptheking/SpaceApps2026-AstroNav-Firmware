#include "display.h"

#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "astronav_protocol.h"
#include "pins.h"

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

// tijdens het versturen 400 kHz (dat kan het scherm prima), daarna terug
// naar de snelheid van de rest van de bus
static Adafruit_SSD1306 oled(128, 64, &Wire, -1, 400000, pins::I2C_FREQ_HZ);
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

// alleen versturen als het beeld anders is: 1 KB over i2c houdt de bus
// ~25 ms bezet, en die is ook nodig voor de sensoren
static void flush() {
  static uint32_t last = 0;
  const uint8_t *buf = oled.getBuffer();
  uint32_t h = 2166136261u;  // FNV-1a
  for (int i = 0; i < 128 * 64 / 8; i++) h = (h ^ buf[i]) * 16777619u;
  if (h == last) return;
  last = h;
  oled.display();
}

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

  centered("Pairing", 0, 1);

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

  wrap("enter code on laptop", 92, 3);

  // ook zonder laptop waarschuwen
  if (alert) strip(alert, true);
  else strip("not connected", false);
  flush();
}

void showStatus(const Telemetry &t, const String &message, const char *alert) {
  if (!found_) return;
  oled.clearDisplay();
  char v[12];

  // hartslag uit de PPG, anders uit de ECG
  if (t.flags & HAS_HEART) snprintf(v, sizeof(v), "%u", t.heart_rate);
  else if (t.hr_ecg) snprintf(v, sizeof(v), "%u", t.hr_ecg);
  else strcpy(v, "--");
  // * = gesimuleerd (demomodus), zodat je op de pols ook ziet wat nep is
  bool simHeart = t.sim_flags & HAS_HEART;
  value(0, simHeart ? "HR*" : "HR", v);

  if (t.flags & HAS_HEART) snprintf(v, sizeof(v), "%u", t.spo2);
  else strcpy(v, "--");
  value(28, simHeart ? "SpO2 %*" : "SpO2 %", v);

  // een melding van het dashboard krijgt de onderste helft, CO2 en de rest
  // komen terug zodra hij bevestigd is (een CO2-alarm staat in de gele strook)
  if (message.length()) {
    oled.drawFastHLine(x0, 54, W, SSD1306_WHITE);
    wrap(message.c_str(), 60, 7);
  } else {
    // mmHg want daar rekent NASA-STD-3001 mee
    if (t.flags & HAS_AIR) snprintf(v, sizeof(v), "%.2f", t.ppco2_avg / 100.0);
    else strcpy(v, "--");
    value(56, (t.sim_flags & HAS_AIR) ? "CO2*mmHg" : "CO2 mmHg", v);

    oled.setTextSize(1);
    oled.setCursor(x0, 86);
    if (t.flags & HAS_PRESSURE)
      oled.printf((t.sim_flags & HAS_PRESSURE) ? "%uhPa*" : "%u hPa", (unsigned)((t.pressure_pa + 50) / 100));
    oled.setCursor(x0, 96);
    if (t.battery_mv) oled.printf(t.sim_battery ? "Bat %u%%*" : "Batt %u%%", t.battery_pct);
    else oled.print("USB");
  }

  if (alert) strip(alert, true);
  else if (message.length()) strip("double tap = seen", false);
  else strip("connected", false);
  flush();
}

void showSensors(uint8_t okBits) {
  if (!found_) return;
  oled.clearDisplay();

  // korte namen, er passen 8 tekens op een regel
  struct { uint8_t bit; const char *name; } list[] = {
    {astronav::OK_MAX30102, "Pulse"}, {astronav::OK_AD8232, "ECG"}, {astronav::OK_MPU6050, "Motion"},
    {astronav::OK_SCD41, "CO2"},      {astronav::OK_BMP180, "Pressure"}, {astronav::OK_GSR, "GSR"},
  };

  int good = 0;
  centered("Sensors", 0, 1);
  oled.drawFastHLine(x0, 10, W, SSD1306_WHITE);
  for (int i = 0; i < 6; i++) {
    bool ok = okBits & list[i].bit;
    good += ok;
    int y = 16 + i * 18;
    oled.setTextSize(1);
    oled.setCursor(x0, y);
    oled.print(list[i].name);
    // vinkje of kruisje, groot genoeg om van een afstand te zien
    oled.setCursor(x0 + W - 12, y + 9);
    oled.print(ok ? "ok" : "--");
    if (!ok) oled.drawFastHLine(x0, y + 16, W, SSD1306_WHITE);
  }

  char line[22];
  snprintf(line, sizeof(line), "%d of 6 sensors ok", good);
  strip(line, good < 6);
  flush();
}

void showBreathing(const char *text, float level) {
  if (!found_) return;
  oled.clearDisplay();

  strip(text, false);
  oled.fillCircle(x0 + W / 2, H / 2, 3 + level * 20, SSD1306_WHITE);
  flush();
}

}
