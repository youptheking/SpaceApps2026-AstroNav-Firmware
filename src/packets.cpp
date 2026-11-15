#include "packets.h"

using namespace astronav;

namespace packets {

// volgnummer per pakkettype. ECG/PPG/IMU/GSR worden in de sample-task
// gemaakt en de rest in loop(), maar nooit hetzelfde type in allebei
static uint16_t seqs[0x21];

void header(Header &h, uint8_t type, uint32_t t_ms) {
  h.type = type;
  h.version = PROTOCOL_VERSION;
  h.seq = type < sizeof(seqs) / sizeof(seqs[0]) ? seqs[type]++ : 0;
  h.t_ms = t_ms;
}

VitalsPacket vitals(const Telemetry &t) {
  VitalsPacket p = {};
  header(p.h, PKT_VITALS, t.uptime_ms);
  bool heart = t.flags & HAS_HEART;
  bool imu = t.flags & HAS_IMU;

  p.hr_ppg = heart ? t.heart_rate : 0;
  p.hr_ecg = t.hr_ecg;
  p.spo2 = heart ? t.spo2 : 0;
  p.perfusion_c10 = heart ? t.perfusion_c10 : 0xFF;
  p.skin_temp_c100 = (t.flags & HAS_SKIN_TEMP) ? t.skin_temp : 0x7FFF;
  p.activity = imu ? t.activity : 0xFF;
  p.tremor_hz_c10 = imu ? t.tremor_hz_c10 : 0xFF;
  p.tremor_amp = imu ? t.tremor : 0xFF;
  p.flags = t.vitals_flags;
  return p;
}

AirPacket air(const Telemetry &t) {
  AirPacket p = {};
  header(p.h, PKT_AIR, t.uptime_ms);
  if (!(t.flags & HAS_AIR)) {
    p.co2_ppm = p.rh_c100 = p.ppco2_c100 = p.ppco2_1h_c100 = 0xFFFF;
    p.temp_c100 = 0x7FFF;
    p.nasa_status = 0xFF;
    return p;
  }

  p.co2_ppm = t.co2;
  p.temp_c100 = t.cabin_temp;
  p.rh_c100 = t.humidity;
  p.ppco2_c100 = t.ppco2;
  p.ppco2_1h_c100 = t.ppco2_avg;

  // [V2 6004] grens 3 mmHg over een uur, OCHMO-doel 2,5 mmHg
  float avg = t.ppco2_avg / 100.0;
  p.nasa_status = avg > CO2_LIMIT_1H_MMHG ? 2 : avg > CO2_TARGET_MMHG ? 1 : 0;
  return p;
}

CabinPacket cabin(const Telemetry &t) {
  CabinPacket p = {};
  header(p.h, PKT_CABIN, t.uptime_ms);
  bool ok = t.flags & HAS_PRESSURE;
  p.pressure_pa = ok ? t.pressure_pa : 0xFFFFFFFF;
  p.pressure_trend = ok ? t.pressure_trend : 0x7FFF;
  // BH1750 geschrapt: alleen in de demomodus een (gesimuleerde) waarde
  p.lux_c10 = t.sim_light ? t.lux_c10 : 0xFFFFFFFF;
  return p;
}

StatusPacket status(const Telemetry &t, uint8_t sensorsOk) {
  StatusPacket p = {};
  header(p.h, PKT_STATUS, t.uptime_ms);
  p.vbat_mv = t.battery_mv ? t.battery_mv : 0xFFFF;
  p.battery_pct = t.battery_mv ? t.battery_pct : 0xFF;
  p.sensors_ok = sensorsOk;
  p.alarms = t.alerts;
  p.fw_major = FW_MAJOR;
  p.fw_minor = FW_MINOR;
  return p;
}

EventPacket event(uint8_t code, uint8_t state, uint8_t severity, int16_t value, uint8_t source) {
  EventPacket p = {};
  header(p.h, PKT_EVENT, millis());
  p.code = code;
  p.state = state;
  p.severity = severity;
  p.value = value;
  p.source = source;
  return p;
}

// zelfde bits als STATUS.sensors_ok, dan kan het dashboard ze naast elkaar leggen
uint8_t simulated(const Telemetry &t) {
  uint8_t g = t.sim_flags, bits = 0;
  if (g & (HAS_HEART | HAS_SKIN_TEMP)) bits |= OK_MAX30102;
  if (g & ECG_LEADS_OK) bits |= OK_AD8232;
  if (g & HAS_IMU) bits |= OK_MPU6050;
  if (g & HAS_AIR) bits |= OK_SCD41;
  if (g & HAS_PRESSURE) bits |= OK_BMP180;
  if (g & HAS_GSR) bits |= OK_GSR;
  if (t.sim_light) bits |= OK_BH1750;
  return bits;
}

SimPacket sim(const Telemetry &t, bool demo) {
  SimPacket p = {};
  header(p.h, PKT_SIM, t.uptime_ms);
  p.demo = demo;
  p.simulated = simulated(t);
  p.extra = t.sim_battery ? SIM_BATTERY : 0;
  return p;
}

}
