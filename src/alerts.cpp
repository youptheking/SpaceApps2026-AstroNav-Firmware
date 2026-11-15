#include "alerts.h"

#include "astronav_protocol.h"

using namespace astronav;

namespace alerts {

// NASA-STD-3001 Vol. 2 Rev F, [V2 6004]: gemiddelde ppCO2 over 1 uur max 3 mmHg.
// Doel 2,5 mmHg komt uit de OCHMO Technical Brief CO2 (zie AIR.nasa_status).
const float MMHG_PER_PA = 1 / 133.322;
const float ISS_PRESSURE_PA = 101325;  // als er geen BMP180 is

// [V2 6012] Crew Health Environmental Limits: werkgebied loopt van 18 °C / 75 % RV
// tot 27 °C / 25 % RV. We gebruiken de omhullende rechthoek.
const float CABIN_TEMP_MIN = 18.0, CABIN_TEMP_MAX = 27.0;
const float CABIN_RH_MIN = 25.0, CABIN_RH_MAX = 75.0;

// [V2 6022] lek: de druk daalt. Eigen keuze: 50 Pa/min (3 kPa per uur) een
// minuut lang. Het weer verandert de druk hooguit een paar Pa per minuut.
const float PRESSURE_DROP_PA_MIN = -50;

// Lichaamswaarden: geen NASA-grenzen, zie de tabel in docs/ble-protocol.md
const uint8_t SPO2_WARN = 94, SPO2_ALARM = 90;  // WHO Pulse Oximetry Training Manual
const uint8_t HR_HIGH_AT_REST = 120, HR_LOW = 40;
const float GSR_RISE = 1.25;    // 25 % boven de eigen basislijn
const float RMSSD_DROP = 0.7;   // 30 % onder de eigen basislijn
const uint8_t BATTERY_LOW_PCT = 15;

struct Rule {
  uint8_t code;
  uint16_t holdSec;  // zo lang moet het kloppen voor het alarm afgaat
  const char *text;
};

// op volgorde van belangrijkheid voor het scherm
static const Rule rules[] = {
  {EV_SPO2_LOW, 30, "Low oxygen (SpO2)"},
  {EV_CO2_NASA_LIMIT, 0, "CO2 > 3 mmHg (1 hour)"},
  {EV_PRESSURE_DROP, 60, "Cabin pressure drop!"},
  {EV_CO2_EARLY, 60, "CO2 now > 3 mmHg"},
  {EV_HR_HIGH, 300, "High heart rate: rest"},
  {EV_HR_LOW, 60, "Low heart rate"},
  {EV_SHOCK, 0, "Impact detected"},
  {EV_CABIN_CLIMATE, 60, "Cabin temp/humidity"},
  {EV_STRESS_HIGH, 600, "Stress: take a break"},
  {EV_BATTERY_LOW, 10, "Battery low: charge"},
  {EV_ECG_LEADS_OFF, 5, "ECG electrode off"},
  {EV_NO_SKIN_CONTACT, 10, "No skin contact"},
};
const size_t RULE_COUNT = sizeof(rules) / sizeof(rules[0]);
const uint32_t CLEAR_AFTER_MS = 5000;

static uint32_t trueSince[RULE_COUNT];
static uint32_t falseSince[RULE_COUNT];
static uint8_t sev[16];
static uint16_t active = 0;

// 1-uursgemiddelde in blokjes van een minuut
static float minuteSum[60];
static uint16_t minuteCount[60];
static uint8_t slot = 0;
static uint32_t slotStart = 0;

static float gsrBaseline = 0, rmssdBaseline = 0;

static void addCo2(float mmHg) {
  if (millis() - slotStart >= 60000) {
    slot = (slot + 1) % 60;
    minuteSum[slot] = 0;
    minuteCount[slot] = 0;
    slotStart = millis();
  }
  minuteSum[slot] += mmHg;
  minuteCount[slot]++;
}

static float co2Average() {
  float sum = 0;
  uint32_t count = 0;
  for (int i = 0; i < 60; i++) {
    sum += minuteSum[i];
    count += minuteCount[i];
  }
  return count ? sum / count : 0;
}

void resetCo2() {
  memset(minuteSum, 0, sizeof(minuteSum));
  memset(minuteCount, 0, sizeof(minuteCount));
  slotStart = millis();
}

static const Rule *find(uint8_t code, size_t &index) {
  for (size_t i = 0; i < RULE_COUNT; i++) {
    if (rules[i].code == code) {
      index = i;
      return &rules[i];
    }
  }
  return nullptr;
}

static void rule(uint8_t code, bool condition, uint8_t severity, int16_t value, Changed changed) {
  size_t i;
  const Rule *r = find(code, i);
  if (!r) return;

  uint32_t now = millis();
  uint16_t bit = 1 << code;

  if (condition) {
    falseSince[i] = 0;
    if (!trueSince[i]) trueSince[i] = now;
    if (now - trueSince[i] < r->holdSec * 1000UL) return;

    if (!(active & bit) || severity > sev[code]) {
      active |= bit;
      sev[code] = severity;
      changed(code, true, severity, value);
    }
  } else {
    trueSince[i] = 0;
    if (!(active & bit)) return;
    if (!falseSince[i]) falseSince[i] = now;
    if (now - falseSince[i] >= CLEAR_AFTER_MS) {
      active &= ~bit;
      changed(code, false, sev[code], value);
    }
  }
}

void update(Telemetry &t, Changed changed) {
  bool heart = t.flags & HAS_HEART;
  bool air = t.flags & HAS_AIR;

  float pa = (t.flags & HAS_PRESSURE) ? t.pressure_pa : ISS_PRESSURE_PA;
  float co2Now = 0, co2Avg = 0;
  if (air) {
    co2Now = t.co2 * 1e-6 * pa * MMHG_PER_PA;
    addCo2(co2Now);
    co2Avg = co2Average();
    t.ppco2 = co2Now * 100;
    t.ppco2_avg = co2Avg * 100;
  }

  // basislijnen volgen langzaam (~10 min), zodat alleen een verandering opvalt
  if (t.flags & HAS_GSR) {
    if (gsrBaseline == 0) gsrBaseline = t.gsr;
    else gsrBaseline += (t.gsr - gsrBaseline) / 600.0;
  }
  if (t.rmssd) {
    if (rmssdBaseline == 0) rmssdBaseline = t.rmssd;
    else rmssdBaseline += (t.rmssd - rmssdBaseline) / 600.0;
  }

  float cabinTemp = t.cabin_temp / 100.0;
  float humidity = t.humidity / 100.0;

  rule(EV_CO2_NASA_LIMIT, air && co2Avg > CO2_LIMIT_1H_MMHG, SEV_ALARM, t.ppco2_avg, changed);
  rule(EV_CO2_EARLY, air && co2Now > CO2_LIMIT_1H_MMHG, SEV_WARNING, t.ppco2, changed);

  uint8_t spo2Sev = t.spo2 < SPO2_ALARM ? SEV_ALARM : SEV_WARNING;
  rule(EV_SPO2_LOW, heart && t.spo2 < SPO2_WARN, spo2Sev, t.spo2, changed);

  // hoge hartslag tijdens sporten is normaal, dus alleen in rust
  rule(EV_HR_HIGH, heart && t.heart_rate > HR_HIGH_AT_REST && (t.flags & HAS_IMU) && t.activity == 0,
       SEV_WARNING, t.heart_rate, changed);
  rule(EV_HR_LOW, heart && t.heart_rate < HR_LOW, SEV_WARNING, t.heart_rate, changed);

  rule(EV_ECG_LEADS_OFF, t.vitals_flags & FLAG_ECG_LEADS_OFF, SEV_INFO, 0, changed);
  rule(EV_NO_SKIN_CONTACT, (t.flags & HAS_SKIN_TEMP) && !(t.vitals_flags & FLAG_SKIN_CONTACT), SEV_INFO, 0,
       changed);
  rule(EV_SHOCK, t.shock_mg > 0, SEV_WARNING, min((int)t.shock_mg, 32767), changed);

  rule(EV_PRESSURE_DROP, (t.flags & HAS_PRESSURE) && t.pressure_trend < PRESSURE_DROP_PA_MIN, SEV_ALARM,
       t.pressure_trend, changed);
  rule(EV_CABIN_CLIMATE, air && (cabinTemp < CABIN_TEMP_MIN || cabinTemp > CABIN_TEMP_MAX ||
                                 humidity < CABIN_RH_MIN || humidity > CABIN_RH_MAX),
       SEV_WARNING, t.cabin_temp, changed);

  // stress: huidgeleiding omhoog, en als de ECG er is ook de HRV omlaag
  bool gsrHigh = (t.flags & HAS_GSR) && t.gsr > gsrBaseline * GSR_RISE;
  bool hrvLow = !t.rmssd || !rmssdBaseline || t.rmssd < rmssdBaseline * RMSSD_DROP;
  rule(EV_STRESS_HIGH, gsrHigh && hrvLow, SEV_WARNING, t.gsr, changed);

  rule(EV_BATTERY_LOW, t.battery_mv && t.battery_pct < BATTERY_LOW_PCT, SEV_WARNING, t.battery_pct, changed);

  t.alerts = active;
}

uint8_t severity(uint8_t code) { return code < 16 ? sev[code] : 0; }

int worst(uint16_t a) {
  int best = -1;
  for (auto &r : rules) {
    if (!(a & (1 << r.code))) continue;
    if (best < 0 || sev[r.code] > sev[best]) best = r.code;
  }
  return best;
}

const char *text(uint16_t a) {
  int code = worst(a);
  if (code < 0) return nullptr;
  size_t i;
  return find(code, i)->text;
}

}
