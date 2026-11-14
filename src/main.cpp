#include <Arduino.h>
#include <Wire.h>

#include "ble_link.h"
#include "display.h"
#include "pins.h"
#include "telemetry.h"

Telemetry telemetry;
String message;  // instructie van het dashboard op het scherm

uint32_t sendInterval = 1000;
uint32_t lastSend = 0;
uint32_t lastScreen = 0;
uint32_t unixAtBoot = 0;
bool ecgStream = false;
bool alarmMuted = false;

void reply(const char *text) {
  Serial.println(text);
  ble::reply(text);
}

void handleCommand(String line) {
  line.trim();
  if (line.isEmpty()) return;

  int space = line.indexOf(' ');
  String cmd = space < 0 ? line : line.substring(0, space);
  String arg = space < 0 ? "" : line.substring(space + 1);
  cmd.toLowerCase();

  if (cmd == "ping") {
    reply("pong");
  } else if (cmd == "info") {
    Serial.printf("%s  code %06lu  %s\n", ble::name(), ble::passkey(),
                  ble::connected() ? "verbonden" : "niet verbonden");
  } else if (cmd == "msg") {
    message = arg;
    reply("ok");
  } else if (cmd == "ack") {
    alarmMuted = true;
    noTone(pins::BUZZER);
    reply("ok");
  } else if (cmd == "time") {
    uint32_t now = strtoul(arg.c_str(), nullptr, 10);
    if (now < 1700000000) return reply("err time");
    unixAtBoot = now - millis() / 1000;
    reply("ok");
  } else if (cmd == "rate") {
    long ms = arg.toInt();
    if (ms < 100 || ms > 60000) return reply("err rate");
    sendInterval = ms;
    reply("ok");
  } else if (cmd == "ecg") {
    ecgStream = arg == "on";
    reply("ok");
  } else if (cmd == "buzz") {
    tone(pins::BUZZER, 2000, 300);
    reply("ok");
  } else {
    reply("err onbekend commando");
  }
}

void updateTelemetry() {
  uint32_t now = millis();
  telemetry.uptime_ms = now;
  telemetry.flags = 0;

  if (unixAtBoot) {
    telemetry.unix_time = unixAtBoot + now / 1000;
    telemetry.flags |= HAS_TIME;
  }

  telemetry.battery_mv = analogReadMilliVolts(pins::BATT) * pins::BATT_DIVIDER;

  // TODO sensoren uitlezen
}

void setup() {
  Serial.begin(115200);
  Wire.begin(pins::I2C_SDA, pins::I2C_SCL, pins::I2C_FREQ_HZ);

  pinMode(pins::ECG_LO_PLUS, INPUT);
  pinMode(pins::ECG_LO_MINUS, INPUT);
  pinMode(pins::ECG_SDN, OUTPUT);
  digitalWrite(pins::ECG_SDN, HIGH);  // SDN laag = ad8232 uit
  pinMode(pins::BUZZER, OUTPUT);

  if (!display::begin()) Serial.println("geen oled gevonden");
  ble::begin();
}

void loop() {
  String line;
  while (ble::readCommand(line)) handleCommand(line);

  // zelfde commando's via usb, handig om te testen
  if (Serial.available()) handleCommand(Serial.readStringUntil('\n'));

  if (millis() - lastSend >= sendInterval) {
    lastSend = millis();
    updateTelemetry();
    ble::send(telemetry);
  }

  if (millis() - lastScreen >= 250) {
    lastScreen = millis();
    if (ble::connected()) display::showStatus(telemetry, message);
    else display::showPairing(ble::name(), ble::passkey());
  }
}
