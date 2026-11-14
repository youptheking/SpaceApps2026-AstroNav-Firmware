// Is het 0.96" scherm een SH1106 in plaats van een SSD1306? Dan zie je hier
// gewoon tekst en een rand, en met de SSD1306-driver alleen flikkerende lijnen.
#include <Adafruit_SH110X.h>
#include <Wire.h>

#include "pins.h"

Adafruit_SH1106G oled(128, 64, &Wire, -1);

void setup() {
  Serial.begin(115200);
  Wire.begin(pins::I2C_SDA, pins::I2C_SCL, pins::I2C_FREQ_HZ);
  bool ok = oled.begin(0x3C, true);
  delay(2000);
  Serial.printf("sh1106 %s\n", ok ? "ok" : "mislukt");

  oled.clearDisplay();
  oled.drawRect(0, 0, 128, 64, SH110X_WHITE);
  oled.setTextColor(SH110X_WHITE);
  oled.setTextSize(2);
  oled.setCursor(10, 10);
  oled.print("SH1106");
  oled.setTextSize(1);
  oled.setCursor(10, 40);
  oled.print("leesbaar = goed");
  oled.display();
}

void loop() {
  delay(1000);
}
