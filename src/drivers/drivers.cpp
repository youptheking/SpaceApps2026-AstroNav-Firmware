#include "drivers.h"

#include "astronav_protocol.h"

using namespace astronav;

// nieuwe sensor: driverbestand maken, functies in drivers.h zetten en hier toevoegen
const Driver drivers[] = {
  {"MAX30102", 0x57, OK_MAX30102, HAS_HEART | HAS_SKIN_TEMP, max30102Begin, max30102Update, max30102Fill},
  {"AD8232", 0, OK_AD8232, ECG_LEADS_OK, ecgBegin, nullptr, ecgFill},
  {"MPU6050", 0x68, OK_MPU6050, HAS_IMU, mpu6050Begin, nullptr, mpu6050Fill},
  {"SCD41", 0x62, OK_SCD41, HAS_AIR, scd41Begin, scd41Update, scd41Fill},
  {"BMP180", 0x77, OK_BMP180, HAS_PRESSURE, bmp180Begin, bmp180Update, bmp180Fill},
  {"GSR", 0, OK_GSR, HAS_GSR, gsrBegin, nullptr, gsrFill},
};

const size_t driverCount = sizeof(drivers) / sizeof(drivers[0]);
