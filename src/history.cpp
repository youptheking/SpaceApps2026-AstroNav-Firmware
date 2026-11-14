#include "history.h"

#include <LittleFS.h>

namespace history {

static const char *PATH = "/history.bin";

static File file;
static uint32_t first = 0, last = 0;

// optellen tijdens de minuut
struct Sums {
  uint32_t start = 0;
  uint32_t unix = 0;
  uint8_t flags = 0, status = 0;
  uint16_t alerts = 0;

  uint32_t n = 0;
  uint32_t heartN = 0, hr = 0, spo2 = 0;
  uint8_t hrMin = 255, hrMax = 0, spo2Min = 255;
  int32_t skinTemp = 0;
  uint32_t skinN = 0;

  uint32_t co2[12] = {};
  uint16_t co2N[12] = {};
  uint16_t ppco2 = 0;
  int32_t cabinTemp = 0;
  uint32_t humidity = 0, airN = 0;

  uint32_t lux = 0, luxN = 0;
  uint32_t gsr = 0, gsrN = 0;
  uint32_t activity = 0, imuN = 0;
  uint8_t tremor = 0;
  uint16_t battery = 0;
};

static Sums sums;

static size_t offsetOf(uint32_t seq) {
  return ((seq - 1) % CAPACITY) * sizeof(HistoryRecord);
}

bool begin() {
  if (!LittleFS.begin(true)) {
    Serial.println("history: LittleFS werkt niet");
    return false;
  }

  // "a" maakt het bestand aan als het er nog niet is (exists() logt dan een fout)
  LittleFS.open(PATH, "a").close();
  file = LittleFS.open(PATH, "r+");

  // hoogste seq zoeken, de ringbuffer onthoudt verder niks
  HistoryRecord r;
  size_t count = file.size() / sizeof(r);
  file.seek(0);
  for (size_t i = 0; i < count; i++) {
    if (file.read((uint8_t *)&r, sizeof(r)) != sizeof(r)) break;
    if (r.version == 1 && r.seq > last) last = r.seq;
  }
  first = last ? last - min((uint32_t)count, last) + 1 : 0;

  sums = Sums();
  sums.start = millis();
  Serial.printf("history: %u minuten opgeslagen (%u-%u)\n", last ? last - first + 1 : 0, first, last);
  return true;
}

static void write() {
  HistoryRecord r;
  r.seq = last + 1;
  r.unix_time = sums.unix;
  r.flags = sums.flags;
  r.status = sums.status;
  r.alerts = sums.alerts;

  if (sums.heartN) {
    r.hr_avg = sums.hr / sums.heartN;
    r.hr_min = sums.hrMin;
    r.hr_max = sums.hrMax;
    r.spo2_avg = sums.spo2 / sums.heartN;
    r.spo2_min = sums.spo2Min;
  }
  if (sums.skinN) r.skin_temp = sums.skinTemp / (int32_t)sums.skinN;

  for (int i = 0; i < 12; i++) {
    if (sums.co2N[i]) r.co2[i] = sums.co2[i] / sums.co2N[i];
  }
  r.ppco2_avg = sums.ppco2;
  if (sums.airN) {
    r.cabin_temp = sums.cabinTemp / (int32_t)sums.airN;
    r.humidity = sums.humidity / sums.airN;
  }
  if (sums.luxN) r.lux = sums.lux / sums.luxN;
  if (sums.gsrN) r.gsr = sums.gsr / sums.gsrN;
  if (sums.imuN) r.activity = sums.activity / sums.imuN;
  r.tremor = sums.tremor;
  r.battery_mv = sums.battery;

  file.seek(offsetOf(r.seq));
  if (file.write((uint8_t *)&r, sizeof(r)) != sizeof(r)) {
    Serial.println("history: schrijven mislukt");
    return;
  }
  file.flush();

  last = r.seq;
  if (!first) first = last;
  if (last - first + 1 > CAPACITY) first = last - CAPACITY + 1;
}

void add(const Telemetry &t) {
  if (!file) return;

  uint32_t now = millis();
  if (now - sums.start >= 60000) {
    if (sums.n) write();
    sums = Sums();
    sums.start = now;
  }

  Sums &s = sums;
  if (s.n == 0 && (t.flags & HAS_TIME)) s.unix = t.unix_time;
  s.n++;
  s.flags |= t.flags;
  s.status |= t.status;
  s.alerts |= t.alerts;
  s.battery = t.battery_mv;

  if (t.flags & HAS_HEART) {
    s.heartN++;
    s.hr += t.heart_rate;
    s.spo2 += t.spo2;
    s.hrMin = min(s.hrMin, t.heart_rate);
    s.hrMax = max(s.hrMax, t.heart_rate);
    s.spo2Min = min(s.spo2Min, t.spo2);
  }
  if (t.flags & HAS_SKIN_TEMP) {
    s.skinN++;
    s.skinTemp += t.skin_temp;
  }
  if (t.flags & HAS_AIR) {
    uint32_t slot = (now - s.start) / 5000;
    if (slot > 11) slot = 11;
    s.co2[slot] += t.co2;
    s.co2N[slot]++;
    s.ppco2 = t.ppco2_avg;
    s.airN++;
    s.cabinTemp += t.cabin_temp;
    s.humidity += t.humidity;
  }
  if (t.flags & HAS_LIGHT) {
    s.luxN++;
    s.lux += t.lux;
  }
  if (t.flags & HAS_GSR) {
    s.gsrN++;
    s.gsr += t.gsr;
  }
  if (t.flags & HAS_IMU) {
    s.imuN++;
    s.activity += t.activity;
    s.tremor = max(s.tremor, t.tremor);
  }
}

uint32_t oldest() { return first; }
uint32_t newest() { return last; }

bool read(uint32_t seq, HistoryRecord &r) {
  if (!file || seq < first || seq > last) return false;
  file.seek(offsetOf(seq));
  if (file.read((uint8_t *)&r, sizeof(r)) != sizeof(r)) return false;
  return r.seq == seq;
}

void wipe() {
  file.close();
  LittleFS.remove(PATH);
  first = last = 0;
  begin();
}

}
