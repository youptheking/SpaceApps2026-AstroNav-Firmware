#include <Arduino.h>
#include <Wire.h>

#include "alerts.h"
#include "ble_link.h"
#include "display.h"
#include "history.h"
#include "pins.h"
#include "sensors.h"
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

// geschiedenis versturen, een paar records per keer
uint32_t historyNext = 0;  // 0 = niks aan het versturen
uint32_t historySent = 0;
uint32_t lastHistorySend = 0;

void reply(const char *text) {
  Serial.println(text);
  ble::reply(text);
}

void printRecord(const HistoryRecord &r) {
  Serial.printf("#%u  t=%u  hr %u (%u-%u)  spo2 %u (min %u)  co2 %u..%u ppm  %.2f mmHg  alarmen 0x%03X%s\n",
                r.seq, r.unix_time, r.hr_avg, r.hr_min, r.hr_max, r.spo2_avg, r.spo2_min,
                r.co2[0], r.co2[11], r.ppco2_avg / 100.0, r.alerts,
                (r.status & STATUS_SIMULATED) ? "  (sim)" : "");
}

void handleHistory(const String &arg) {
  if (arg.isEmpty() || arg == "info") {
    char buf[40];
    snprintf(buf, sizeof(buf), "history %u %u", history::oldest(), history::newest());
    reply(buf);
  } else if (arg == "stop") {
    historyNext = 0;
    reply("ok");
  } else if (arg == "wipe") {
    historyNext = 0;
    history::wipe();
    reply("ok");
  } else if (arg.startsWith("dump")) {
    // alleen voor usb: laatste n minuten als tekst
    int n = arg.length() > 5 ? arg.substring(5).toInt() : 5;
    uint32_t from = history::newest() >= (uint32_t)n ? history::newest() - n + 1 : 1;
    HistoryRecord r;
    for (uint32_t seq = max(from, history::oldest()); seq && seq <= history::newest(); seq++) {
      if (history::read(seq, r)) printRecord(r);
    }
  } else {
    if (!ble::connected()) return reply("err niet verbonden");
    uint32_t from = strtoul(arg.c_str(), nullptr, 10);
    historyNext = max(from, history::oldest());
    historySent = 0;
    if (!historyNext || historyNext > history::newest()) {
      historyNext = 0;
      reply("history klaar 0");
    }
  }
}

void sendHistoryChunk() {
  if (!historyNext || millis() - lastHistorySend < 20) return;
  lastHistorySend = millis();

  if (!ble::connected()) {
    historyNext = 0;
    return;
  }

  size_t perPacket = min((size_t)6, (ble::maxPayload() - 1) / sizeof(HistoryRecord));
  if (perPacket == 0) {
    historyNext = 0;
    return reply("err mtu te klein");
  }

  uint8_t buf[1 + 6 * sizeof(HistoryRecord)];
  uint8_t count = 0;
  while (count < perPacket && historyNext <= history::newest()) {
    HistoryRecord r;
    if (history::read(historyNext, r)) {
      memcpy(buf + 1 + count * sizeof(r), &r, sizeof(r));
      count++;
    }
    historyNext++;
  }

  if (count) {
    buf[0] = count;
    ble::sendHistory(buf, 1 + count * sizeof(HistoryRecord));
    historySent += count;
  }

  if (historyNext > history::newest()) {
    historyNext = 0;
    char done[32];
    snprintf(done, sizeof(done), "history klaar %u", historySent);
    reply(done);
  }
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
    Serial.printf("lux %u  gsr %u  act %u  tremor %u  batt %u%s\n",
                  t.lux, t.gsr, t.activity, t.tremor, t.battery_mv,
                  (t.status & STATUS_BREATHING) ? "  (ademhaling)" : "");
    Serial.printf("echt 0x%02X  gesimuleerd 0x%02X\n", t.flags & ~t.sim_flags & ~HAS_TIME, t.sim_flags);
    const char *a = alerts::text(t.alerts);
    Serial.printf("alarmen 0x%03X  %s%s\n", t.alerts, a ? a : "-",
                  (t.status & STATUS_MUTED) ? "  (stil)" : "");
  } else if (cmd == "oled") {
    // scherm opnieuw zoeken, handig als het bij het opstarten niet reageerde
    reply(display::begin() ? "oled ok" : "oled niet gevonden");
  } else if (cmd == "sensors") {
    sensors::print();
    Serial.printf("%-9s 0x3C  %s\n", "OLED", display::found() ? "ok" : "niet gevonden");
  } else if (cmd == "history") {
    handleHistory(arg);
  } else if (cmd == "reboot") {
    reply("ok");
    delay(100);
    ESP.restart();
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
  if (ecgStream) telemetry.status |= STATUS_ECG_ON;

  sensors::fill(telemetry);

  // wat er niet is, simuleren (ecg alleen als de stream aan staat)
  if (simulate) {
    uint8_t missing = (sensors::ALL | ECG_LEADS_OK) & ~sensors::found();
    if (!ecgStream) missing &= ~ECG_LEADS_OK;
    sim::fill(telemetry, missing);
    // onder 2,5 V hangt er geen accu aan
    if (telemetry.battery_mv < 2500) sim::fillBattery(telemetry);
  }
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
  bool real = sensors::found() & ECG_LEADS_OK;
  if (!real && !simulate) return;

  // inhalen als loop() even bezig was (bijv. met het scherm)
  while ((int32_t)(micros() - nextEcg) >= 0) {
    ecgBuf[ecgCount++] = real ? analogReadMilliVolts(pins::ECG_OUT) : sim::ecg(nextEcg);
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
  // SDN als ingang, de pull-up op het ad8232-bordje houdt hem hoog (= aan)
  pinMode(pins::ECG_SDN, INPUT);
  pinMode(pins::BUZZER, OUTPUT);

  if (!display::begin()) Serial.println("geen oled gevonden");
  sensors::begin();
  history::begin();
  ble::begin();
}

void loop() {
  String line;
  while (ble::readCommand(line)) handleCommand(line);

  // zelfde commando's via usb, handig om te testen
  if (Serial.available()) handleCommand(Serial.readStringUntil('\n'));

  sensors::update();
  sampleEcg();

  // alarmen elke seconde bijwerken, ook als `rate` langzamer is
  if (millis() - lastTick >= min(sendInterval, 1000u)) {
    lastTick = millis();
    uint16_t before = telemetry.alerts;
    updateTelemetry();
    history::add(telemetry);

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
  sendHistoryChunk();

  // tijdens de ademhaling vaker, anders loopt de cirkel schokkerig
  uint32_t screenInterval = breatheEnd ? 50 : 250;
  if (millis() - lastScreen >= screenInterval) {
    lastScreen = millis();
    updateScreen();
  }
}
