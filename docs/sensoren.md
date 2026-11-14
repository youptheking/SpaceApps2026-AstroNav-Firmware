# Sensoren aansluiten

De firmware zoekt bij het opstarten zelf welke sensoren er zijn. Wat hij vindt, levert echte data. Wat ontbreekt, wordt gesimuleerd zolang `sim` aan staat (standaard). Je kunt de breakouts dus één voor één op het breadboard zetten.

Controleren via de USB-monitor (115200):

```
sensors   -> welke sensoren gevonden zijn
show      -> meetwaarden, met "echt" en "gesimuleerd" als bitmask
```

Na het aansluiten van een sensor: herstarten (`reboot` of de resetknop), want er wordt alleen bij het opstarten gezocht.

## Bedrading (XIAO ESP32-S3)

Alle I2C-sensoren komen samen op één bus:

| XIAO | Sensor |
|---|---|
| 3V3 | VCC / VIN |
| GND | GND |
| D4 | SDA |
| D5 | SCL |

De meeste breakouts hebben zelf pull-ups. Zonder één breakout met pull-ups werkt de bus niet; voeg dan 4,7 kΩ van SDA en SCL naar 3V3 toe.

| Sensor | Adres | Driver | Wat |
|---|---|---|---|
| SCD40 | 0x62 | `src/drivers/scd40.cpp` | CO2, cabinetemperatuur, luchtvochtigheid. Eerste meting na 5 s. |
| MAX30102 | 0x57 | `src/drivers/max30102.cpp` | Hartslag en SpO2. Eerste waarde na ~5 s pols tegen de sensor. |
| MCP9808 | 0x18 | `src/drivers/mcp9808.cpp` | Huidtemperatuur |
| BH1750 | 0x23 | `src/drivers/bh1750.cpp` | Omgevingslicht |
| MPU-6050 | 0x68 | `src/drivers/mpu6050.cpp` | Beweging, activiteit, tremor. Eerste waarde na 2 s. |
| OLED | 0x3C | `src/display.cpp` | Scherm |

Analoog:

| XIAO | Sensor |
|---|---|
| D0 | AD8232 OUTPUT |
| D1 | AD8232 LO− |
| D2 | AD8232 LO+ |
| D3 | AD8232 SDN |
| D8 | Grove GSR SIG |
| D7 | Piezobuzzer (+) |
| D9 | Batterijspanning via deler |

GSR en AD8232 zijn niet automatisch te vinden. Zet `ANALOG_FRONTEND` in `include/pins.h` op `true` als ze aangesloten zijn.

## Goed om te weten

- **MAX30102:** de sensor moet stil tegen de huid zitten; bewegen geeft rommel. Als er geen pols gemeten wordt (IR-signaal te laag), stuurt het horloge niks in plaats van een verkeerde waarde, dus ook geen vals SpO2-alarm. Het algoritme is dat van Maxim uit de SparkFun-bibliotheek (25 Hz, venster van 4 s).
- **MPU-6050:** de Adafruit-bibliotheek controleert het chip-ID. Sommige goedkope klonen (eigenlijk een MPU-6500) worden daardoor niet herkend en blijven dan gesimuleerd. Tremor en activiteit worden berekend in `src/motion.cpp`; het algoritme is zonder IMU te testen met `pio run -e test_motion -t upload`.
- **Tremor:** rms van de versnelling in de 4–12 Hz band, per as, in mg. Score 0–100 is 1 punt per mg. Lopen (1–2 Hz) telt niet mee.
- **Activiteit:** rms van de versnelling boven 0,5 Hz. Score 0–100 is 1 punt per 5 mg.
- **Grove GSR:** eerst afstellen met de potmeter op de module (zonder huidcontact ongeveer halve voedingsspanning). De omrekening naar µS komt uit de Grove-wiki en moet nog nagekeken worden met de echte module.
- **MCP9808 aan de pols:** meet huidtemperatuur, die is lager dan de kerntemperatuur. Het koortsalarm is dus alleen een indicatie (zie [alarmen.md](alarmen.md)).
- **SCD40:** krijgt de cabinedruk mee (`CABIN_PRESSURE_MMHG` in `src/alerts.h`), zodat ppm en de mmHg-berekening voor [V2 6004] bij elkaar passen.

## Nieuwe sensor toevoegen

1. Driverbestand in `src/drivers/` met een `begin`, (optioneel) `update` en `fill`.
2. De functies in `src/drivers/drivers.h` zetten.
3. Een regel in de lijst in `src/drivers/drivers.cpp`, met het I2C-adres en de sensorgroep.
