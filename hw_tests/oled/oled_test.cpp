// oled-test zonder Wire: zet het scherm aan met i2c in software en tekent
// een rand en strepen. Licht hij op, dan zijn scherm en bedrading goed.
#include <Arduino.h>

#include "pins.h"

const uint8_t BUS_SDA = pins::I2C_SDA, BUS_SCL = pins::I2C_SCL;
const uint8_t ADDR = 0x3C;
const int HALF = 5;

void low(uint8_t pin) { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); }
void release(uint8_t pin) { pinMode(pin, INPUT); }

void sclHigh() {
  release(BUS_SCL);
  uint32_t start = micros();
  while (!digitalRead(BUS_SCL) && micros() - start < 2000) {}
}

void start() {
  release(BUS_SDA);
  sclHigh();
  delayMicroseconds(HALF);
  low(BUS_SDA);
  delayMicroseconds(HALF);
  low(BUS_SCL);
}

void stop() {
  low(BUS_SDA);
  delayMicroseconds(HALF);
  sclHigh();
  delayMicroseconds(HALF);
  release(BUS_SDA);
  delayMicroseconds(HALF);
}

bool sendByte(uint8_t b) {
  for (int bit = 7; bit >= 0; bit--) {
    if (b & (1 << bit)) release(BUS_SDA);
    else low(BUS_SDA);
    delayMicroseconds(HALF);
    sclHigh();
    delayMicroseconds(HALF);
    low(BUS_SCL);
  }
  release(BUS_SDA);
  delayMicroseconds(HALF);
  sclHigh();
  bool ack = !digitalRead(BUS_SDA);
  delayMicroseconds(HALF);
  low(BUS_SCL);
  return ack;
}

// control 0x00 = commando's, 0x40 = beelddata
bool send(uint8_t control, const uint8_t *data, size_t n) {
  start();
  bool ok = sendByte(ADDR << 1) && sendByte(control);
  for (size_t i = 0; ok && i < n; i++) ok = sendByte(data[i]);
  stop();
  return ok;
}

// standaard opstartreeks voor een SSD1306 van 128x32
const uint8_t init_cmds[] = {
  0xAE, 0xD5, 0x80, 0xA8, 0x1F, 0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x00,
  0xA1, 0xC8, 0xDA, 0x02, 0x81, 0x8F, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF,
};

void draw(int frame) {
  const uint8_t window[] = {0x21, 0, 127, 0x22, 0, 3};
  send(0x00, window, sizeof(window));

  // 4 pagina's van 8 pixels hoog, per keer 16 bytes
  uint8_t buf[16];
  for (int page = 0; page < 4; page++) {
    for (int x0 = 0; x0 < 128; x0 += 16) {
      for (int i = 0; i < 16; i++) {
        int x = x0 + i;
        uint8_t col = ((x + frame) / 8) % 2 ? 0x0F : 0xF0;
        if (x == 0 || x == 127) col = 0xFF;
        if (page == 0) col |= 0x01;
        if (page == 3) col |= 0x80;
        buf[i] = col;
      }
      send(0x40, buf, sizeof(buf));
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  bool ok = send(0x00, init_cmds, sizeof(init_cmds));
  Serial.printf("oled init %s\n", ok ? "ok" : "geen ack");
}

void loop() {
  static int frame = 0;
  draw(frame++);
  if (frame % 20 == 0) Serial.printf("frame %d\n", frame);
  delay(100);
}
