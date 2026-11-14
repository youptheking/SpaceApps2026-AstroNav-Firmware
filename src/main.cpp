#include <Arduino.h>
#include <Wire.h>

#include "ble_link.h"
#include "display.h"
#include "pins.h"
#include "sim.h"
#include "telemetry.h"

const uint32_t ECG_PERIOD_US = 4000;  // 250 Hz
const size_t ECG_BATCH = 25;          // 10 pakketjes per seconde

Telemetry telemetry;
String message;  // instructie van het dashboard op het scherm

uint32_t sendInterval = 1000;
uint32_t lastSend = 0;
uint32_t lastScreen = 0;
uint32_t unixAtBoot = 0;
bool ecgStream = false;
bool simulate = true;  // uitzetten zodra de sensoren erin zitten
bool alarmMuted = false;

uint32_t breatheStart = 0;
uint32_t breatheEnd = 0;  // 0 = geen oefening

int16_t ecgBuf[ECG_BATCH];
size_t ecgCount = 0;
uint32_t nextEcg = 0;

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
    ecgCount = 0;
    nextEcg = micros();
    reply("ok");
  } else if (cmd == "sim") {
    simulate = arg == "on";
    reply("ok");
  } else if (cmd == "breathe") {
    if (arg == "off") {
      breatheEnd = 0;
    } else {
      long minutes = arg.isEmpty() ? 2 : arg.toInt();
      if (minutes < 1 || minutes > 30) return reply("err breathe");
      breatheStart = millis();
      breatheEnd = breatheStart + minutes * 60000;
    }
    reply("ok");
  } else if (cmd == "show") {
    const Telemetry &t = telemetry;
    Serial.printf("hr %u  spo2 %u  temp %.2f  co2 %u  lux %u  gsr %u  act %u  tremor %u  batt %u%s%s\n",
                  t.heart_rate, t.spo2, t.skin_temp / 100.0, t.co2, t.lux, t.gsr,
                  t.activity, t.tremor, t.battery_mv,
                  (t.status & STATUS_SIMULATED) ? "  (sim)" : "",
                  (t.status & STATUS_BREATHING) ? "  (ademhaling)" : "");
  } else if (cmd == "buzz") {
    tone(pins::BUZZER, 2000, 300);
    reply("ok");
  } else {
    reply("err onbekend commando");
  }
}

void updateTelemetry() {
  uint32_t now = millis();
  telemetry = Telemetry();
  telemetry.uptime_ms = now;

  if (unixAtBoot) {
    telemetry.unix_time = unixAtBoot + now / 1000;
    telemetry.flags |= HAS_TIME;
  }

  telemetry.battery_mv = analogReadMilliVolts(pins::BATT) * pins::BATT_DIVIDER;

  // TODO sensoren uitlezen

  if (simulate) sim::fill(telemetry);
  if (breatheEnd) telemetry.status |= STATUS_BREATHING;
}

// 4 s in, 4 s vast, 6 s uit
void showBreathing() {
  uint32_t t = (millis() - breatheStart) % 14000;
  if (t < 4000) display::showBreathing("Adem in", t / 4000.0);
  else if (t < 8000) display::showBreathing("Vasthouden", 1.0);
  else display::showBreathing("Adem uit", 1.0 - (t - 8000) / 6000.0);
}

void updateScreen() {
  if (breatheEnd && (int32_t)(millis() - breatheEnd) >= 0) {
    breatheEnd = 0;
    reply("breathe klaar");
  }

  if (breatheEnd) showBreathing();
  else if (ble::connected()) display::showStatus(telemetry, message);
  else display::showPairing(ble::name(), ble::passkey());
}

void sampleEcg() {
  if (!ecgStream) return;

  // inhalen als loop() even bezig was (bijv. met het scherm)
  while ((int32_t)(micros() - nextEcg) >= 0) {
    ecgBuf[ecgCount++] = simulate ? sim::ecg(nextEcg) : analogReadMilliVolts(pins::ECG_OUT);
    nextEcg += ECG_PERIOD_US;

    if (ecgCount == ECG_BATCH) {
      ble::sendEcg(ecgBuf, ecgCount);
      ecgCount = 0;
    }
  }
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

  sampleEcg();

  if (millis() - lastSend >= sendInterval) {
    lastSend = millis();
    updateTelemetry();
    ble::send(telemetry);
  }

  // tijdens de ademhaling vaker, anders loopt de cirkel schokkerig
  uint32_t screenInterval = breatheEnd ? 50 : 250;
  if (millis() - lastScreen >= screenInterval) {
    lastScreen = millis();
    updateScreen();
  }
}
