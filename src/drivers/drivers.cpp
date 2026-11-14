#include "drivers.h"

// nieuwe sensor: driverbestand maken, functies in drivers.h zetten en hier toevoegen
const Driver drivers[] = {
  {"SCD40", 0x62, HAS_AIR, scd40Begin, scd40Update, scd40Fill},
  {"MAX30102", 0x57, HAS_HEART, max30102Begin, max30102Update, max30102Fill},
};

const size_t driverCount = sizeof(drivers) / sizeof(drivers[0]);
