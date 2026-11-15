#include <Arduino.h>
#include <Wire.h>

#include "alerts.h"
#include "astronav_protocol.h"
#include "ble_link.h"
#include "buzzer.h"
#include "display.h"
#include "drivers/drivers.h"
#include "history.h"
#include "packets.h"
#include "pins.h"
#include "sampler.h"
#include "sensors.h"
#include "sim.h"
#include "telemetry.h"

using namespace astronav;

const uint32_t STATUS_EVERY_MS = 10000;
const uint32_t BREATHE_MS = 2 * 60000;

// teksten bij control 0x04 (message_id), zie docs/ble-protocol.md. In het Engels,
// afgebroken voor het staande scherm (8 tekens breed).
const char *MESSAGES[] = {
  "Take a break",
  "Start breath- ing exer- cise 2 min",
  "CO2 high: check venti- lation",
  "Low oxygen: stop activ- ity",
  "Pressure falling: check cabin",
  "Sleep deficit: plan rest",
  "Elec- trode off: check chest",
  "Battery low: charge",
};
const size_t MESSAGE_COUNT = sizeof(MESSAGES) / sizeof(MESSAGES[0]);

Telemetry telemetry;
// demomodus: wat ontbreekt (licht, accu, een sensor die wegvalt) krijgt nepwaarden,
// en het SIM-pakket zegt welke. Uit met `sim off` of control 0x12.
bool simulate = pins::DEMO_MODE;
uint32_t unixAtBoot = 0;
uint16_t muted = 0;  // alarmen die bevestigd zijn, buzzer stil tot ze weer opnieuw beginnen

// melding van het dashboard (control 0x04)
struct {
  bool active = false;
  uint8_t code = 0, severity = 0, message = 0;
} remote;

uint32_t breatheStart = 0;
uint32_t breatheEnd = 0;  // 0 = geen oefening

uint32_t lastTick = 0, lastStatus = 0, lastScreen = 0, lastAirSim = 0;
uint16_t statusAlerts = 0;
uint8_t statusSim = 0, statusExtra = 0;

// geschiedenis versturen, een paar records per keer
uint32_t historyNext = 0;  // 0 = niks aan het versturen
uint32_t lastHistorySend = 0;

void reply(const char *text) {
  Serial.println(text);
}

void sendEvent(uint8_t code, bool on, uint8_t severity, int16_t value, uint8_t source) {
  Serial.printf("event %u %s ernst %u waarde %d bron %u\n", code, on ? "aan" : "uit", severity, value, source);
  ble::sendEvent(packets::event(code, on, severity, value, source));
}

// [V2 6022] lokaal en op afstand waarschuwen: elk alarm gaat meteen naar de laptop
void onAlert(uint8_t code, bool on, uint8_t severity, int16_t value) {
  if (on) muted &= ~(1 << code);  // nieuw of erger: weer laten piepen
  sendEvent(code, on, severity, value, SRC_WRIST);
}

// astronaut heeft het gezien (dubbeltik of `ack`): alles stil
void acknowledge() {
  int code = remote.active ? remote.code : alerts::worst(telemetry.alerts & ~muted);
  muted |= telemetry.alerts;
  remote.active = false;
  buzzer::set(-1);
  if (code >= 0) sendEvent(EV_ACKNOWLEDGED, true, SEV_INFO, code, SRC_ASTRONAUT);
  else reply("niks te bevestigen");
}

void startBreathing(uint32_t ms) {
  breatheStart = millis();
  breatheEnd = breatheStart + ms;
}

void showRemote(uint8_t code, uint8_t severity, uint8_t message) {
  remote.active = true;
  remote.code = code;
  remote.severity = severity;
  remote.message = message < MESSAGE_COUNT ? message : 0;
  if (remote.message == 1) startBreathing(BREATHE_MS);
  if (severity == SEV_INFO) buzzer::beep();
  // bevestigen dat de melding getoond wordt
  sendEvent(code, true, severity, 0, SRC_DASHBOARD);
}

void sendHistoryInfo() {
  uint8_t buf[9] = {0};
  uint32_t range[2] = {history::oldest(), history::newest()};
  memcpy(buf + 1, range, sizeof(range));
  ble::sendHistory(buf, sizeof(buf));
}

// eerste byte = aantal records, 0 = alleen het bereik (oudste, nieuwste)
void sendHistoryChunk() {
  if (!historyNext || millis() - lastHistorySend < 20) return;
  lastHistorySend = millis();

  if (!ble::connected()) {
    historyNext = 0;
    return;
  }

  uint8_t buf[1 + 4 * sizeof(HistoryRecord)];
  size_t perPacket = min((size_t)4, (ble::maxPayload() - 1) / sizeof(HistoryRecord));
  uint8_t count = 0;
  while (count < perPacket && historyNext <= history::newest()) {
    HistoryRecord r;
    if (history::read(historyNext, r)) memcpy(buf + 1 + count++ * sizeof(r), &r, sizeof(r));
    historyNext++;
  }

  if (count) {
    buf[0] = count;
    ble::sendHistory(buf, 1 + count * sizeof(HistoryRecord));
  }
  // klaar: nog een keer het bereik, dan weet het dashboard dat het af is
  if (historyNext > history::newest()) {
    historyNext = 0;
    sendHistoryInfo();
  }
}

void requestHistory(uint32_t from) {
  if (!from) return sendHistoryInfo();
  historyNext = max(from, history::oldest());
  if (!historyNext || historyNext > history::newest()) {
    historyNext = 0;
    sendHistoryInfo();
  }
}

void setTime(uint32_t unix) {
  if (unix > 1700000000) unixAtBoot = unix - millis() / 1000;
}

void handleControl(const uint8_t *buf, size_t len) {
  if (!len) return;
  uint32_t arg32 = 0;
  if (len >= 5) memcpy(&arg32, buf + 1, 4);

  switch (buf[0]) {
    case CMD_BUZZER_TEST:
      buzzer::beep();
      break;
    case CMD_STREAMS:
      if (len >= 2) sampler::setRawStreams(buf[1]);
      break;
    case CMD_CLEAR_ALERT:
      if (len < 2) break;
      if (buf[1] < 16) muted |= 1 << buf[1];
      if (remote.active && remote.code == buf[1]) remote.active = false;
      break;
    case CMD_ALERT:
      if (len >= 4) showRemote(buf[1], buf[2], buf[3]);
      break;
    case CMD_HISTORY:
      requestHistory(arg32);
      break;
    case CMD_DEMO:
      if (len >= 2) simulate = buf[1];
      break;
    case CMD_SET_TIME:
      setTime(arg32);
      break;
  }
}

void printShow() {
  const Telemetry &t = telemetry;
  Serial.printf("hr %u (ecg %u)  spo2 %u  pi %.1f%%  chip %.2f C  rmssd %u\n", t.heart_rate, t.hr_ecg, t.spo2,
                t.perfusion_c10 / 10.0, t.skin_temp / 100.0, t.rmssd);
  Serial.printf("co2 %u ppm  %.2f mmHg (1u %.2f)  cabine %.1f C %.0f%%  druk %u Pa (%d Pa/min)\n", t.co2,
                t.ppco2 / 100.0, t.ppco2_avg / 100.0, t.cabin_temp / 100.0, t.humidity / 100.0, t.pressure_pa,
                t.pressure_trend);
  Serial.printf("act %u (%u mg)  tremor %u mg %.1f Hz  gsr %.2f uS  accu %u mV %u%%  vlaggen 0x%02X\n", t.activity,
                t.activity_mg, t.tremor, t.tremor_hz_c10 / 10.0, t.gsr / 100.0, t.battery_mv, t.battery_pct,
                t.vitals_flags);
  Serial.printf("gemeten 0x%02X  gesimuleerd 0x%02X  stream %s  verloren %u\n", t.flags & ~t.sim_flags & ~HAS_TIME,
                t.sim_flags, sampler::rawStreams() ? "aan" : "uit", sampler::dropped());
  Serial.printf("demo %s  licht %.1f lux%s  accu%s\n", simulate ? "aan" : "uit", t.lux_c10 / 10.0,
                t.sim_light ? " (sim)" : "", t.sim_battery ? " (sim)" : "");
  const char *a = alerts::text(t.alerts);
  Serial.printf("alarmen 0x%04X  %s%s\n", t.alerts, a ? a : "-", (t.status & STATUS_MUTED) ? "  (stil)" : "");
}

void printRecord(const HistoryRecord &r) {
  Serial.printf("#%u  t=%u  hr %u (%u-%u)  spo2 %u (min %u)  co2 %u..%u ppm  %.2f mmHg  %.1f hPa  alarmen 0x%04X\n",
                r.seq, r.unix_time, r.hr_avg, r.hr_min, r.hr_max, r.spo2_avg, r.spo2_min, r.co2[0], r.co2[11],
                r.ppco2_avg / 100.0, r.pressure / 10.0, r.alerts);
}

// tekstcommando's via usb, om te testen zonder dashboard
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
    Serial.printf("%s  code %06lu  %s  firmware %u.%u\n", ble::name(), ble::passkey(),
                  ble::connected() ? "verbonden" : ble::linked() ? "verbonden, wacht op code" : "niet verbonden",
                  packets::FW_MAJOR, packets::FW_MINOR);
  } else if (cmd == "sensors") {
    sensors::print();
    Serial.printf("%-9s 0x3C  %s\n", "OLED", display::found() ? "ok" : "niet gevonden");
  } else if (cmd == "bmp") {
    Serial.printf("bmp180: %.1f C  %u Pa\n", bmp180Temperature(), telemetry.pressure_pa);
  } else if (cmd == "show") {
    printShow();
  } else if (cmd == "ack") {
    acknowledge();
  } else if (cmd == "alert") {
    // alert <code> <ernst> <tekst-id>, zoals control 0x04 van het dashboard
    int a = arg.indexOf(' '), b = arg.lastIndexOf(' ');
    if (a < 0 || b <= a) return reply("err alert <code> <ernst> <tekst>");
    showRemote(arg.substring(0, a).toInt(), arg.substring(a + 1, b).toInt(), arg.substring(b + 1).toInt());
  } else if (cmd == "streams") {
    sampler::setRawStreams(arg != "off");
    reply("ok");
  } else if (cmd == "time") {
    setTime(strtoul(arg.c_str(), nullptr, 10));
    reply(unixAtBoot ? "ok" : "err time");
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
    if (arg == "off") breatheEnd = 0;
    else startBreathing((arg.isEmpty() ? 2 : constrain(arg.toInt(), 1, 30)) * 60000);
    reply("ok");
  } else if (cmd == "history") {
    if (arg.isEmpty() || arg == "info") {
      Serial.printf("history %u %u\n", history::oldest(), history::newest());
    } else if (arg == "wipe") {
      history::wipe();
      reply("ok");
    } else {
      // history <n>: laatste n minuten als tekst
      uint32_t n = max(1L, arg.toInt());
      uint32_t from = history::newest() > n ? history::newest() - n + 1 : 1;
      HistoryRecord r;
      for (uint32_t seq = max(from, history::oldest()); seq && seq <= history::newest(); seq++) {
        if (history::read(seq, r)) printRecord(r);
      }
    }
  } else if (cmd == "reg") {
    // reg <adres> <register> [aantal], hex. Om een sensor na te kijken
    char *end;
    uint8_t addr = strtoul(arg.c_str(), &end, 16);
    uint8_t reg = strtoul(end, &end, 16);
    int n = constrain(strtol(end, nullptr, 10), 1, 32);
    Wire.beginTransmission(addr);
    Wire.write(reg);
    uint8_t err = Wire.endTransmission(false);
    int got = Wire.requestFrom(addr, (uint8_t)n);
    Serial.printf("0x%02X reg 0x%02X (fout %u):", addr, reg, err);
    while (Wire.available()) Serial.printf(" %02X", Wire.read());
    Serial.printf("  (%d bytes)\n", got);
  } else if (cmd == "oled") {
    reply(display::begin() ? "oled ok" : "oled niet gevonden");
  } else if (cmd == "buzz") {
    buzzer::beep();
    reply("ok");
  } else if (cmd == "reboot") {
    reply("ok");
    delay(100);
    ESP.restart();
  } else {
    reply("err onbekend commando");
  }
}

// LiPo: spanning naar een grof percentage
uint8_t batteryPercent(uint16_t mv) {
  const uint16_t volts[] = {3300, 3500, 3600, 3700, 3800, 3900, 4000, 4100, 4200};
  const uint8_t pct[] = {0, 5, 15, 30, 50, 65, 80, 90, 100};
  if (mv <= volts[0]) return 0;
  for (int i = 1; i < 9; i++) {
    if (mv <= volts[i]) return pct[i - 1] + (pct[i] - pct[i - 1]) * (mv - volts[i - 1]) / (volts[i] - volts[i - 1]);
  }
  return 100;
}

void updateTelemetry() {
  uint32_t now = millis();
  telemetry = Telemetry();
  telemetry.uptime_ms = now;

  if (unixAtBoot) {
    telemetry.unix_time = unixAtBoot + now / 1000;
    telemetry.flags |= HAS_TIME;
  }

  // onder 2,5 V hangt er geen accu aan, alleen usb
  uint16_t mv = pins::HAS_BATTERY ? analogReadMilliVolts(pins::BATT) * pins::BATT_DIVIDER : 0;
  if (mv > 2500) {
    telemetry.battery_mv = mv;
    telemetry.battery_pct = batteryPercent(mv);
  }

  sensors::fill(telemetry);
  if (simulate) {
    sim::fill(telemetry, sensors::ALL & ~sensors::groups());
    sim::fillLight(telemetry);
    if (!telemetry.battery_mv) {
      sim::fillBattery(telemetry);
      telemetry.battery_pct = batteryPercent(telemetry.battery_mv);
    }
  }
  if (breatheEnd) telemetry.status |= STATUS_BREATHING;

  alerts::update(telemetry, onAlert);

  muted &= telemetry.alerts;  // opgeloste alarmen vergeten
  if (muted) telemetry.status |= STATUS_MUTED;
}

// ernstigste melding die nog niet bevestigd is bepaalt het patroon.
// Info-meldingen van de pols zelf (elektrode los, geen contact) piepen niet.
void updateBuzzer() {
  int level = -1;
  uint16_t loud = telemetry.alerts & ~muted;
  for (int code = 0; code < 16; code++) {
    if (loud & (1 << code)) level = max(level, (int)alerts::severity(code));
  }
  if (remote.active) level = max(level, (int)remote.severity);
  buzzer::set(level);
  buzzer::update();
}

// 4 s in, 4 s vast, 6 s uit
void showBreathing() {
  uint32_t t = (millis() - breatheStart) % 14000;
  if (t < 4000) display::showBreathing("Breathe in", t / 4000.0);
  else if (t < 8000) display::showBreathing("Hold", 1.0);
  else display::showBreathing("Breathe out", 1.0 - (t - 8000) / 6000.0);
}

void updateScreen() {
  if (breatheEnd && (int32_t)(millis() - breatheEnd) >= 0) {
    breatheEnd = 0;
    if (remote.active && remote.message == 1) remote.active = false;
  }

  // bij het opstarten en als er een sensor bijkomt of wegvalt: 5 s laten
  // zien wat er werkt, dan hoef je daar geen laptop voor te pakken
  static uint8_t shownSensors = 0xFF;
  static uint32_t sensorsUntil = 0;
  if (sensors::okBits() != shownSensors) {
    shownSensors = sensors::okBits();
    sensorsUntil = millis() + 5000;
  }

  const char *alert = alerts::text(telemetry.alerts);
  String message = remote.active ? MESSAGES[remote.message] : "";
  if ((int32_t)(millis() - sensorsUntil) < 0) display::showSensors(shownSensors);
  else if (breatheEnd) showBreathing();
  else if (ble::connected()) display::showStatus(telemetry, message, alert);
  else display::showPairing(ble::name(), ble::passkey(), alert);
}

void sendStatus() {
  uint8_t ok = sensors::okBits() | (display::found() ? OK_OLED : 0);
  StatusPacket s = packets::status(telemetry, ok);
  ble::sendData(&s, sizeof(s));

  // meteen erachteraan: welke waarden nep zijn, zodat het dashboard dat erbij kan zetten
  SimPacket m = packets::sim(telemetry, simulate);
  ble::sendData(&m, sizeof(m));

  lastStatus = millis();
  statusAlerts = telemetry.alerts;
  statusSim = m.simulated;
  statusExtra = m.extra;
}

// is er iets veranderd aan wat er gesimuleerd wordt? Dan meteen opnieuw sturen
bool simChanged() {
  uint8_t extra = telemetry.sim_battery ? SIM_BATTERY : 0;
  return packets::simulated(telemetry) != statusSim || extra != statusExtra;
}

// eens per seconde: alles bijwerken en de langzame pakketten versturen
void tick() {
  bool airNew = sensors::has(OK_SCD41) && scd41TakeNew();
  updateTelemetry();
  history::add(telemetry);

  VitalsPacket v = packets::vitals(telemetry);
  ble::sendData(&v, sizeof(v));
  CabinPacket c = packets::cabin(telemetry);
  ble::sendData(&c, sizeof(c));

  // AIR bij elke meting van de SCD41 (elke 5 s), gesimuleerd ook elke 5 s
  bool airSim = (telemetry.sim_flags & HAS_AIR) && millis() - lastAirSim >= 5000;
  if (airSim) lastAirSim = millis();
  if (airNew || airSim) {
    AirPacket a = packets::air(telemetry);
    ble::sendData(&a, sizeof(a));
  }

  if (millis() - lastStatus >= STATUS_EVERY_MS || telemetry.alerts != statusAlerts || simChanged()) sendStatus();
}

// als een module halverwege een byte bleef hangen (bijv. na een reset),
// houdt hij SDA laag. Met 9 klokpulsen laat hij weer los.
void recoverBus() {
  pinMode(pins::I2C_SDA, INPUT_PULLUP);
  pinMode(pins::I2C_SCL, INPUT_PULLUP);
  for (int i = 0; i < 9 && !digitalRead(pins::I2C_SDA); i++) {
    pinMode(pins::I2C_SCL, OUTPUT);
    digitalWrite(pins::I2C_SCL, LOW);
    delayMicroseconds(5);
    pinMode(pins::I2C_SCL, INPUT_PULLUP);
    delayMicroseconds(5);
  }
}

void setup() {
  Serial.begin(115200);
  recoverBus();
  Wire.begin(pins::I2C_SDA, pins::I2C_SCL, pins::I2C_FREQ_HZ);
  Wire.setTimeOut(50);

  buzzer::begin();
  if (!display::begin()) Serial.println("geen oled gevonden");
  sensors::begin();
  history::begin();
  ble::begin();
  sampler::begin();

  // hangt loop() langer dan 5 s (bijv. de i2c-bus), dan herstart hij vanzelf
  enableLoopWDT();
}

void loop() {
  uint8_t buf[sizeof(PpgPacket)];
  size_t len;
  while (ble::readControl(buf, len)) handleControl(buf, len);

  // zelfde commando's via usb, handig om te testen
  if (Serial.available()) handleCommand(Serial.readStringUntil('\n'));

  // ruwe streams en RR uit de sample-task doorsturen
  for (int i = 0; i < 16 && sampler::pop(buf, len); i++) ble::sendData(buf, len);

  sensors::update();
  if (sensors::has(OK_MPU6050) && mpu6050TakeDoubleTap()) acknowledge();

  // net ontgrendeld: meteen STATUS en SIM, dan weet de app dat de code goed was
  static bool wasUnlocked = false;
  if (ble::connected() && !wasUnlocked) sendStatus();
  wasUnlocked = ble::connected();

  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    tick();
  }

  updateBuzzer();
  sendHistoryChunk();

  // tijdens de ademhaling vaker, anders loopt de cirkel schokkerig
  uint32_t screenInterval = breatheEnd ? 50 : 200;
  if (millis() - lastScreen >= screenInterval) {
    lastScreen = millis();
    updateScreen();
  }
}
