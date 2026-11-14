#include "drivers.h"

// nieuwe sensor: driverbestand maken, functies in drivers.h zetten en hier toevoegen
const Driver drivers[] = {
  {"SCD40", 0x62, HAS_AIR, scd40Begin, scd40Update, scd40Fill},
};

const size_t driverCount = sizeof(drivers) / sizeof(drivers[0]);
