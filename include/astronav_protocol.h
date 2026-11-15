// AstroNav BLE-protocol v1 – gedeelde definities voor de firmware.
// Bron van waarheid: docs/ble-protocol.md. Pas dat document aan bij elke wijziging hier.
#pragma once
#include <stdint.h>

namespace astronav {

constexpr uint8_t PROTOCOL_VERSION = 1;

constexpr const char* DEVICE_NAME       = "AstroNav-Wrist";
constexpr const char* SERVICE_UUID      = "2075864c-7ef8-4db3-9901-ff4606218bb1";
constexpr const char* CHAR_DATA_UUID    = "8b4d589b-2465-4b5e-9a2c-219eb307cabe";  // notify
constexpr const char* CHAR_EVENTS_UUID  = "631cd0ae-17e9-48b0-beec-431b6a4ca732";  // indicate
constexpr const char* CHAR_CONTROL_UUID = "4419bc3d-3bfd-40d1-8da1-6bcae37219b5";  // write
constexpr const char* CHAR_HISTORY_UUID = "d7a1e5c2-3b4f-4c8a-9e61-0b7f2a9c4d13";  // notify, uitbreiding

enum PacketType : uint8_t {
  PKT_ECG = 0x01, PKT_PPG = 0x02, PKT_IMU = 0x03, PKT_GSR = 0x04,
  PKT_VITALS = 0x05, PKT_RR = 0x06, PKT_AIR = 0x07, PKT_CABIN = 0x08,
  PKT_STATUS = 0x09, PKT_EVENT = 0x20,
};

enum EventCode : uint8_t {
  EV_CO2_NASA_LIMIT = 0, EV_CO2_EARLY = 1, EV_SPO2_LOW = 2, EV_HR_HIGH = 3,
  EV_HR_LOW = 4, EV_ECG_LEADS_OFF = 5, EV_NO_SKIN_CONTACT = 6, EV_SHOCK = 7,
  EV_PRESSURE_DROP = 8, EV_BATTERY_LOW = 9, EV_STRESS_HIGH = 10, EV_ACKNOWLEDGED = 11,
  EV_CABIN_CLIMATE = 12,  // uitbreiding: V2 6012/6022, cabinetemperatuur of -vocht buiten het werkgebied
};

enum Severity : uint8_t { SEV_INFO = 0, SEV_WARNING = 1, SEV_ALARM = 2 };
enum EventSource : uint8_t { SRC_WRIST = 0, SRC_DASHBOARD = 1, SRC_ASTRONAUT = 2 };

enum ControlCmd : uint8_t {
  CMD_BUZZER_TEST = 0x01, CMD_STREAMS = 0x02, CMD_CLEAR_ALERT = 0x03, CMD_ALERT = 0x04,
  // uitbreidingen, zie "Uitbreidingen" in docs/ble-protocol.md
  CMD_HISTORY = 0x10,  // u32 vanaf_seq, 0 = alleen het bereik opvragen
  CMD_SET_TIME = 0x11,  // u32 unix-tijd, voor de tijdstempels in de geschiedenis
};

// VITALS.flags
constexpr uint8_t FLAG_SKIN_CONTACT    = 1 << 0;
constexpr uint8_t FLAG_MOTION_ARTIFACT = 1 << 1;
constexpr uint8_t FLAG_ECG_LEADS_OFF   = 1 << 2;
constexpr uint8_t FLAG_SHOCK           = 1 << 3;

// STATUS.sensors_ok
constexpr uint8_t OK_MAX30102 = 1 << 0, OK_AD8232 = 1 << 1, OK_MPU6050 = 1 << 2, OK_SCD41 = 1 << 3,
                  OK_BMP180 = 1 << 4, OK_BH1750 = 1 << 5, OK_OLED = 1 << 6, OK_GSR = 1 << 7;

// NASA-STD-3001 Vol. 2, V2 6004: ppCO2 1-uursgemiddelde <= 3 mmHg (zie docs/alarmen.md)
constexpr float CO2_LIMIT_1H_MMHG = 3.0f;
constexpr float CO2_TARGET_MMHG   = 2.5f;   // OCHMO Technical Brief CO2, lange termijn

#pragma pack(push, 1)
struct Header { uint8_t type, version; uint16_t seq; uint32_t t_ms; };

struct EcgPacket    { Header h; uint16_t mv[25]; };                  // 58 bytes
struct PpgPacket    { Header h; uint32_t red[10], ir[10]; };         // 88 bytes
struct ImuSample    { int16_t ax, ay, az, gx, gy, gz; };
struct ImuPacket    { Header h; ImuSample s[5]; };                   // 68 bytes
struct GsrPacket    { Header h; uint16_t nS[10]; };                  // 28 bytes
struct VitalsPacket { Header h; uint8_t hr_ppg, hr_ecg, spo2, perfusion_c10;
                      int16_t skin_temp_c100; uint8_t activity, tremor_hz_c10,
                      tremor_amp, flags; uint16_t reserved; };       // 20 bytes
struct AirPacket    { Header h; uint16_t co2_ppm; int16_t temp_c100;
                      uint16_t rh_c100, ppco2_c100, ppco2_1h_c100;
                      uint8_t nasa_status; uint16_t reserved; };     // 21 bytes
struct CabinPacket  { Header h; uint32_t pressure_pa; int16_t pressure_trend;
                      uint32_t lux_c10; uint16_t reserved; };        // 20 bytes
struct StatusPacket { Header h; uint16_t vbat_mv; uint8_t battery_pct, sensors_ok;
                      uint16_t alarms; uint8_t fw_major, fw_minor; }; // 16 bytes
struct EventPacket  { Header h; uint8_t code, state, severity; int16_t value;
                      uint8_t source; };                           // 14 bytes
#pragma pack(pop)

static_assert(sizeof(Header) == 8, "Header");
static_assert(sizeof(EcgPacket) == 58, "ECG");
static_assert(sizeof(PpgPacket) == 88, "PPG");
static_assert(sizeof(ImuPacket) == 68, "IMU");
static_assert(sizeof(GsrPacket) == 28, "GSR");
static_assert(sizeof(VitalsPacket) == 20, "VITALS");
static_assert(sizeof(AirPacket) == 21, "AIR");
static_assert(sizeof(CabinPacket) == 20, "CABIN");
static_assert(sizeof(StatusPacket) == 16, "STATUS");
static_assert(sizeof(EventPacket) == 14, "EVENT");

}  // namespace astronav
