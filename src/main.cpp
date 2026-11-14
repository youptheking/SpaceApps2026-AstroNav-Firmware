#include <Arduino.h>
#include <Wire.h>

#include "alerts.h"
#include "ble_link.h"
#include "display.h"
#include "pins.h"
#include "sim.h"
#include "telemetry.h"

const uint32_t ECG_PERIOD_US = 4000;  // 250 Hz
const size_t ECG_BATCH = 25;          // 10 pakketjes per seconde
const uint32_t BEEP_EVERY_MS = 3000;

Telemetry telemetry;
String message;  // instructie van het dashboard op het scherm

uint32_t sendInterval = 1000;
uint32_t lastTick = 0;
uint32_t lastSend = 0;
uint32_t lastScreen = 0;
uint32_t lastBeep = 0;
uint32_t unixAtBoot = 0;
bool ecgStream = false;
bool simulate = true;  // uitzetten zodra de sensoren erin zitten
uint16_t mutedAlerts = 0;  // alarmen die met `ack` bevestigd zijn

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
    mutedAlerts = telemetry.alerts;
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
    if (arg == "on" || arg == "off") {
      simulate = arg == "on";
      return reply("ok");
    }
    // sim co2 4500, sim spo2 88, ... (0 = automatisch)
    int sp = arg.indexOf(' ');
    String field = arg.substring(0, sp);
    if (sp < 0 || !sim::fix(field, arg.substring(sp + 1).toFloat())) return reply("err sim");
    if (field == "co2") alerts::resetCo2();
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
    Serial.printf("hr %u  spo2 %u  temp %.2f  co2 %u ppm (1u gem %.2f mmHg)  cabine %.1f C %.0f%%\n",
                  t.heart_rate, t.spo2, t.skin_temp / 100.0, t.co2, t.ppco2_avg / 100.0,
                  t.cabin_temp / 100.0, t.humidity / 100.0);
    Serial.printf("lux %u  gsr %u  act %u  tremor %u  batt %u%s%s\n",
                  t.lux, t.gsr, t.activity, t.tremor, t.battery_mv,
                  (t.status & STATUS_SIMULATED) ? "  (sim)" : "",
                  (t.status & STATUS_BREATHING) ? "  (ademhaling)" : "");
    const char *a = alerts::text(t.alerts);
    Serial.printf("alarmen 0x%03X  %s%s\n", t.alerts, a ? a : "-",
                  (t.status & STATUS_MUTED) ? "  (stil)" : "");
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

  if (ecgStream) {
    telemetry.status |= STATUS_ECG_ON;
    // LO+ of LO- hoog = elektrode los
    if (!digitalRead(pins::ECG_LO_PLUS) && !digitalRead(pins::ECG_LO_MINUS))
      telemetry.flags |= ECG_LEADS_OK;
  }

  // TODO sensoren uitlezen

  if (simulate) sim::fill(telemetry);
  if (breatheEnd) telemetry.status |= STATUS_BREATHING;

  alerts::update(telemetry);

  mutedAlerts &= telemetry.alerts;  // opgeloste alarmen vergeten
  if (mutedAlerts) telemetry.status |= STATUS_MUTED;
}

// [V2 6022] lokaal waarschuwen: piepen bij kritieke alarmen tot `ack`
void updateBuzzer() {
  uint16_t loud = telemetry.alerts & ~mutedAlerts;
  if (!alerts::critical(loud)) return;
  if (millis() - lastBeep < BEEP_EVERY_MS) return;
  lastBeep = millis();
  tone(pins::BUZZER, 2500, 400);
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

  const char *alert = alerts::text(telemetry.alerts);
  if (breatheEnd) showBreathing();
  else if (ble::connected()) display::showStatus(telemetry, message, alert);
  else display::showPairing(ble::name(), ble::passkey(), alert);
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

  // alarmen elke seconde bijwerken, ook als `rate` langzamer is
  if (millis() - lastTick >= min(sendInterval, 1000u)) {
    lastTick = millis();
    uint16_t before = telemetry.alerts;
    updateTelemetry();

    // [V2 6022] op afstand waarschuwen: bij een nieuw of opgelost alarm meteen versturen
    bool changed = telemetry.alerts != before;
    if (changed) {
      const char *a = alerts::text(telemetry.alerts);
      Serial.printf("alarm: 0x%03X %s\n", telemetry.alerts, a ? a : "opgelost");
    }
    if (changed || millis() - lastSend >= sendInterval) {
      lastSend = millis();
      ble::send(telemetry);
    }
  }

  updateBuzzer();

  // tijdens de ademhaling vaker, anders loopt de cirkel schokkerig
  uint32_t screenInterval = breatheEnd ? 50 : 250;
  if (millis() - lastScreen >= screenInterval) {
    lastScreen = millis();
    updateScreen();
  }
}
