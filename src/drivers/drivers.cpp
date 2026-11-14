#include "drivers.h"

// nieuwe sensor: driverbestand maken, functies in drivers.h zetten en hier toevoegen
const Driver drivers[] = {
  {"SCD40", 0x62, HAS_AIR, scd40Begin, scd40Update, scd40Fill},
  {"MAX30102", 0x57, HAS_HEART, max30102Begin, max30102Update, max30102Fill},
  {"MCP9808", 0x18, HAS_SKIN_TEMP, mcp9808Begin, mcp9808Update, mcp9808Fill},
  {"BH1750", 0x23, HAS_LIGHT, bh1750Begin, bh1750Update, bh1750Fill},
  {"MPU6050", 0x68, HAS_IMU, mpu6050Begin, mpu6050Update, mpu6050Fill},
};

const size_t driverCount = sizeof(drivers) / sizeof(drivers[0]);
