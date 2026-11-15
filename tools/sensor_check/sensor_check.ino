// AstroNav – sensorcheck voor het breadboard en de eerste test van de PCB
//
// Generated with Claude (SI) based on Team AstroNav's design and pinout
// (docs/pinout.md). Reviewed and tested by the team. See README "Gebruik van AI (SI)".
//
// Geen extra libraries nodig, alleen de ESP32 Arduino-core (board: XIAO_ESP32S3).
// Open de Serial Monitor op 115200 baud.
//
// Wat het doet:
//   1. Meet de spanning op SDA/SCL in rust (vangt het 1,8 V-probleem van de MAX30102)
//   2. Scant de I2C-bus en vergelijkt met de verwachte adressen
//   3. Leest chip-ID's uit (MAX30102, MPU-6050, BMP180)
//   4. Piept één keer op de buzzer (D7)
//   5. Toont daarna elke 500 ms: ECG, LO+/LO−, GSR en accuspanning

#include <Arduino.h>
#include <Wire.h>

struct Device { uint8_t addr; const char* name; };
const Device EXPECTED[] = {
  {0x3C, "OLED"}, {0x57, "MAX30102"},
  {0x62, "SCD41"},  {0x68, "MPU-6050"}, {0x77, "BMP180"},
};
const size_t N_EXPECTED = sizeof(EXPECTED) / sizeof(EXPECTED[0]);

// Leest één register. Geeft -1 terug als het apparaat niet antwoordt.
int readReg(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return -1;
  if (Wire.requestFrom(addr, (uint8_t)1) != 1) return -1;
  return Wire.read();
}

bool present(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

void checkId(uint8_t addr, uint8_t reg, uint8_t expected, const char* name) {
  int id = readReg(addr, reg);
  if (id < 0) {
    Serial.printf("  %-9s geen antwoord\n", name);
  } else if (id == expected) {
    Serial.printf("  %-9s ID 0x%02X  OK\n", name, id);
  } else {
    Serial.printf("  %-9s ID 0x%02X  (verwacht 0x%02X) -> ander type chip?\n", name, id, expected);
  }
}

// Wachttijd bij het opstarten voordat de sensoren worden aangesproken.
// Geeft de voeding en de buffercondensatoren (100 µF bij de SCD41) tijd om te
// stabiliseren, en de sensoren tijd om zelf op te starten.
constexpr uint32_t STARTUP_DELAY_MS = 2000;

void setup() {
  // Buzzer-pin direct laag zetten, zodat hij niet zweeft tijdens het wachten
  pinMode(D7, OUTPUT);
  digitalWrite(D7, LOW);
  delay(STARTUP_DELAY_MS);

  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}
  delay(200);
  Serial.println("\n=== AstroNav sensorcheck ===");

  analogSetAttenuation(ADC_11db);  // ADC-bereik tot ~3,1 V

  // 1. Spanning op de bus in rust, vóór Wire.begin()
  pinMode(D4, INPUT);
  pinMode(D5, INPUT);
  delay(10);
  uint32_t sda = analogReadMilliVolts(D4);
  uint32_t scl = analogReadMilliVolts(D5);
  Serial.printf("\n[1] I2C in rust: SDA %lu mV, SCL %lu mV  ", sda, scl);
  if (sda < 2500 || scl < 2500) {
    Serial.println("-> TE LAAG");
    Serial.println("    ~1800 mV: pull-ups van de MAX30102 hangen aan 1,8 V.");
    Serial.println("    ~0 mV:    geen pull-ups op de bus (of SDA/SCL niet aangesloten).");
  } else {
    Serial.println("-> OK");
  }

  // 2. I2C-scan
  Wire.begin(D4, D5);
  Wire.setClock(100000);
  Serial.println("\n[2] I2C-scan:");
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    if (!present(a)) continue;
    found++;
    const char* name = "onbekend";
    for (size_t i = 0; i < N_EXPECTED; i++) if (EXPECTED[i].addr == a) name = EXPECTED[i].name;
    Serial.printf("  0x%02X  %s\n", a, name);
  }
  Serial.printf("  %d apparaat/apparaten gevonden\n", found);
  for (size_t i = 0; i < N_EXPECTED; i++) {
    if (!present(EXPECTED[i].addr)) Serial.printf("  ONTBREEKT: 0x%02X %s\n", EXPECTED[i].addr, EXPECTED[i].name);
  }

  // 3. Chip-ID's
  Serial.println("\n[3] Chip-ID's:");
  checkId(0x57, 0xFF, 0x15, "MAX30102");  // PART_ID
  checkId(0x68, 0x75, 0x68, "MPU-6050");  // WHO_AM_I (MPU-6500 geeft 0x70)
  checkId(0x77, 0xD0, 0x55, "BMP180");    // chip-ID
  Serial.println("  (SCD41 en OLED hebben geen simpel ID-register; aanwezig = OK)");

  // 4. Buzzer
  Serial.println("\n[4] Buzzer op D7: je hoort nu een piep");
  tone(D7, 4000, 200);
  delay(300);
  noTone(D7);

  // 5. Analoge pinnen
  pinMode(D1, INPUT);  // AD8232 LO−
  pinMode(D2, INPUT);  // AD8232 LO+
  pinMode(D3, INPUT);  // AD8232 SDN: als ingang laten, de pull-up op het bordje houdt hem hoog
  Serial.println("\n[5] Live (elke 500 ms): ECG op D0, LO−/LO+ (1 = elektrode los), SDN, GSR op D9, accu op D8");
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last < 500) return;
  last = millis();

  uint32_t ecg  = analogReadMilliVolts(D0);
  int loMin     = digitalRead(D1);
  int loPlus    = digitalRead(D2);
  int sdn       = digitalRead(D3);
  uint32_t gsr  = analogReadMilliVolts(D9);
  uint32_t vbat = analogReadMilliVolts(D8) * 2;  // spanningsdeler 2× 200 kΩ

  Serial.printf("ECG %4lu mV | LO- %d LO+ %d | SDN %d | GSR %4lu mV | accu %4lu mV\n",
                ecg, loMin, loPlus, sdn, gsr, vbat);
}
