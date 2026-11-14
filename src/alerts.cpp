#include "alerts.h"

namespace alerts {

// NASA-STD-3001 Vol. 2 Rev F, [V2 6004]: gemiddelde ppCO2 over 1 uur max 3 mmHg.
const float CO2_LIMIT_MMHG = 3.0;
// OCHMO Technical Brief CO2: onder 2,5 mmHg blijft het risico op hoofdpijn < 1 %.
// Wij gebruiken dat als vroege waarschuwing.
const float CO2_WARN_MMHG = 2.5;
// De SCD40 meet in ppm, de norm rekent in mmHg. ISS-cabine = 1 atm = 760 mmHg.
// Bij een andere cabinedruk (bijv. Artemis) dit aanpassen.
const float CABIN_PRESSURE_MMHG = 760.0;

// [V2 6012] Crew Health Environmental Limits: werkgebied loopt van 18 °C / 75 % RV
// tot 27 °C / 25 % RV. We gebruiken de omhullende rechthoek, dat is iets ruimer
// dan de figuur in de norm.
const float CABIN_TEMP_MIN = 18.0, CABIN_TEMP_MAX = 27.0;
const float CABIN_RH_MIN = 25.0, CABIN_RH_MAX = 75.0;

// Lichaamswaarden: niet uit NASA-STD-3001, gangbare klinische grenzen.
const uint8_t SPO2_MIN = 92;
const uint8_t HR_MAX_AT_REST = 120;
const uint8_t HR_MIN = 40;
const float SKIN_TEMP_MAX = 37.5;  // pols, dus alleen een indicatie voor koorts
const float GSR_RISE = 1.25;       // 25 % boven de eigen basislijn
const float BATT_MIN_V = 3.5;

struct Rule {
  uint16_t bit;
  uint16_t holdSec;  // zo lang moet het kloppen voor het alarm afgaat
  const char *text;
};

// op volgorde van belangrijkheid, de eerste die actief is komt op het scherm
static const Rule rules[] = {
  {ALERT_LOW_SPO2, 10, "SpO2 laag: rustig aan"},
  {ALERT_HIGH_CO2, 5, "CO2 > 3 mmHg (1 uur)"},
  {ALERT_HIGH_HR, 30, "Hartslag hoog: rust"},
  {ALERT_LOW_HR, 30, "Hartslag laag"},
  {ALERT_FEVER, 300, "Temp hoog: meld arts"},
  {ALERT_CO2_RISING, 5, "CO2 > 2.5 mmHg"},
  {ALERT_CABIN, 60, "Cabine temp/vocht!"},
  {ALERT_STRESS, 15, "Stress: adem rustig"},
  {ALERT_LEADS_OFF, 5, "ECG elektrodes los"},
  {ALERT_LOW_BATT, 10, "Batterij bijna leeg"},
};
const size_t RULE_COUNT = sizeof(rules) / sizeof(rules[0]);
const uint32_t CLEAR_AFTER_MS = 5000;

static uint32_t trueSince[RULE_COUNT];
static uint32_t falseSince[RULE_COUNT];
static uint16_t active = 0;

// 1-uursgemiddelde in blokjes van een minuut
static float minuteSum[60];
static uint16_t minuteCount[60];
static uint8_t slot = 0;
static uint32_t slotStart = 0;

static float gsrBaseline = 0;

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

static void check(size_t i, bool condition) {
  uint32_t now = millis();
  uint16_t bit = rules[i].bit;

  if (condition) {
    falseSince[i] = 0;
    if (!trueSince[i]) trueSince[i] = now;
    if (now - trueSince[i] >= rules[i].holdSec * 1000UL) active |= bit;
  } else {
    trueSince[i] = 0;
    if (!falseSince[i]) falseSince[i] = now;
    if (now - falseSince[i] >= CLEAR_AFTER_MS) active &= ~bit;
  }
}

static void rule(uint16_t bit, bool condition) {
  for (size_t i = 0; i < RULE_COUNT; i++) {
    if (rules[i].bit == bit) check(i, condition);
  }
}

void update(Telemetry &t) {
  bool heart = t.flags & HAS_HEART;
  bool air = t.flags & HAS_AIR;

  float co2Avg = 0;
  if (air) {
    addCo2(t.co2 * CABIN_PRESSURE_MMHG / 1e6);
    co2Avg = co2Average();
    t.ppco2_avg = co2Avg * 100;
  }

  float cabinTemp = t.cabin_temp / 100.0;
  float humidity = t.humidity / 100.0;

  if (t.flags & HAS_GSR) {
    // basislijn volgt langzaam (~10 min), zodat alleen een stijging opvalt
    if (gsrBaseline == 0) gsrBaseline = t.gsr;
    else gsrBaseline += (t.gsr - gsrBaseline) / 600.0;
  }

  rule(ALERT_LOW_SPO2, heart && t.spo2 < SPO2_MIN);
  rule(ALERT_HIGH_HR, heart && t.heart_rate > HR_MAX_AT_REST && t.activity < 30);
  rule(ALERT_LOW_HR, heart && t.heart_rate < HR_MIN);
  rule(ALERT_FEVER, (t.flags & HAS_SKIN_TEMP) && t.skin_temp / 100.0 > SKIN_TEMP_MAX);
  rule(ALERT_HIGH_CO2, air && co2Avg > CO2_LIMIT_MMHG);
  rule(ALERT_CO2_RISING, air && co2Avg > CO2_WARN_MMHG && co2Avg <= CO2_LIMIT_MMHG);
  rule(ALERT_CABIN, air && (cabinTemp < CABIN_TEMP_MIN || cabinTemp > CABIN_TEMP_MAX ||
                            humidity < CABIN_RH_MIN || humidity > CABIN_RH_MAX));
  rule(ALERT_STRESS, (t.flags & HAS_GSR) && t.gsr > gsrBaseline * GSR_RISE);
  // onder 2,5 V hangt er geen accu aan (alleen usb)
  rule(ALERT_LOW_BATT, t.battery_mv > 2500 && t.battery_mv < BATT_MIN_V * 1000);
  // alleen als de ECG aan staat, anders is "los" logisch
  rule(ALERT_LEADS_OFF, (t.status & STATUS_ECG_ON) && !(t.flags & ECG_LEADS_OK));

  t.alerts = active;
}

const char *text(uint16_t a) {
  for (auto &r : rules) {
    if (a & r.bit) return r.text;
  }
  return nullptr;
}

bool critical(uint16_t a) {
  return a & (ALERT_LOW_SPO2 | ALERT_HIGH_CO2);
}

}
