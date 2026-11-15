# Sensoren

De firmware zoekt bij het opstarten zelf welke sensoren er zijn, en daarna elke 10 s opnieuw naar de sensoren die ontbreken. Een sensor die later wakker wordt of een contact dat terugkomt, wordt zo vanzelf weer gebruikt, zonder herstart.

**Op het horloge zelf:** bij het opstarten, en elke keer als er een sensor bijkomt of wegvalt, staat 5 s lang het sensorscherm op het OLED ("5 of 6 sensors ok").

**Via de USB-monitor (115200):**

```
sensors   -> welke sensoren gevonden zijn
show      -> alle meetwaarden en actieve alarmen
reg 68 75 -> een i2c-register uitlezen (hier WHO_AM_I van de MPU-6050)
```

Ontbreekt er een sensor, dan levert die niks (ongeldig in de pakketten). Met `sim on` worden ontbrekende sensoren gesimuleerd, handig om zonder hardware te testen. Standaard staat dat uit.

## Bedrading (XIAO ESP32-S3)

Pinindeling volgens het schema van de PCB. In Arduino altijd de `D0`–`D10`-namen gebruiken, de netnamen in het schema volgen de D-nummers en niet de GPIO-nummers (zie `include/pins.h`).

### I2C

| XIAO | |
|---|---|
| 3V3 | VCC / VIN van alle modules |
| GND | GND van alle modules |
| D4 | SDA |
| D5 | SCL |

**Pull-ups: 2× 4,3–4,7 kΩ van SDA en SCL naar 3V3 (R5/R6) zijn nodig.** Zonder deze weerstanden kwam SCL er 2–7 µs over om hoog te worden, terwijl 100 kHz hooguit 1 µs toestaat. De hardware-I2C van de ESP32 las dan verkeerde bits: onzinwaarden van de BMP180, mislukte reads van de MPU-6050 en een bus die vastliep. In software (die geduldig wacht) werkte alles wel, en dat was het bewijs. Doel: 2–5 kΩ totaal. Isoleer de weerstanden: het metalen kapje van de XIAO is GND.

De bus draait op 100 kHz. Het OLED schakelt tijdens het versturen even naar 400 kHz.

| Sensor | Adres | Driver | Wat |
|---|---|---|---|
| MAX30102 | 0x57 | `src/drivers/max30102.cpp` | PPG op 100 Hz, hartslag, SpO2, perfusie, chiptemperatuur. Eerste waarde na ~5 s met de pols stil tegen de sensor. |
| MPU-6050 | 0x68 | `src/drivers/mpu6050.cpp` | Beweging op 50 Hz: activiteit, tremor, schok, dubbeltik. Eerste waarde na 2 s. |
| SCD41 | 0x62 | `src/drivers/scd41.cpp` | CO2, cabinetemperatuur, luchtvochtigheid. Elke 5 s een meting. |
| BMP180 | 0x77 | `src/drivers/bmp180.cpp` | Cabinedruk en de trend over een minuut, voor ppCO2 in mmHg en het lekalarm. |
| OLED 0.96" | 0x3C of 0x3D | `src/display.cpp` | 128×64, 90° gedraaid (staand 64×128), tweekleurig. |
| ~~BH1750~~ | – | – | Geschrapt: de pinvolgorde van de footprint klopte niet. `CABIN.lux_c10` blijft `0xFFFFFFFF`. |
| ~~MCP9808~~ | – | – | Zit niet op de PCB. De huidtemperatuur komt van de chiptemperatuur van de MAX30102 (alleen als trend). |

### Analoog en de rest

| XIAO | |
|---|---|
| D0 | AD8232 OUTPUT (ECG, 250 Hz) |
| D1 | AD8232 LO− |
| D2 | AD8232 LO+ |
| D3 | AD8232 SDN (ingang, de pull-up op het bordje houdt hem aan) |
| D7 | Piezobuzzer via 100 Ω |
| D8 | Accuspanning via 2× 200 kΩ (BAT_SW+), `× 2` |
| D9 | GSR: 3,3 V → koperen vlak → huid → vlak → D9, met 220 kΩ naar GND (10 Hz) |
| D6, D10 | vrij |

GSR en AD8232 zijn niet automatisch te vinden. Ze zitten op de PCB, dus `ANALOG_FRONTEND` in `include/pins.h` staat op `true`. Zet hem op `false` om zonder te testen.

## Goed om te weten

- **MAX30102:** moet stil tegen de huid zitten, bewegen geeft rommel (dan staat de artefact-vlag aan). Zonder huidcontact (IR-signaal te laag) stuurt het horloge geen hartslag of SpO2 in plaats van een verkeerde waarde, dus ook geen vals SpO2-alarm. Hij meet op 100 Hz voor de PPG-stream; voor het Maxim-algoritme (SparkFun-bibliotheek) worden telkens 4 samples gemiddeld tot 25 Hz, venster van 4 s.
- **MAX30102 op de PCB:** de footprint klopt niet. Op jumpers deed hij het wel, op de PCB-aansluiting niet. Op de paarse modules is de volgorde VIN, SCL, SDA, INT, IRD, RD, GND. Bij sommige modules zit er een soldeerbruggetje voor 1,8/3,3 V op de achterkant: op 3,3 V zetten.
- **MPU-6050:** eigen driver via de registers, zodat ook klonen werken (WHO_AM_I 0x68, 0x70, 0x71 of 0x73). De eerste module rekte de klok (clock stretching) en daar kan de hardware-I2C van de ESP32 slecht tegen; de reservemodule werkt goed. Lukt het lezen 10 keer achter elkaar niet, dan pauzeert hij een seconde, zodat hij de bus niet bezet houdt.
- **Tremor:** rms van de versnelling in de 4–12 Hz band, per as, in mg. Lopen (1–2 Hz) telt niet mee. De frequentie komt uit de nuldoorgangen. Zonder IMU te testen met `pio run -e test_motion -t upload`.
- **Dubbeltik:** twee tikken tussen 0,12 en 0,5 s uit elkaar, boven 700 mg zonder zwaartekracht, en alleen als de pols verder stil is. Afgesteld op het prototype; bij een andere behuizing opnieuw proberen (`TAP_MG` in `src/motion.cpp`).
- **AD8232:** de elektrodes komen via de 3,5 mm-jack. LO+ of LO− hoog = elektrode los, dan worden de R-pieken gereset.
- **GSR:** de firmware rekent de spanning om naar geleiding (`src/drivers/gsr.cpp`). Onder 30 mV is er geen huidcontact.
- **SCD41:** start opnieuw als hij 30 s niks meet. Krijgt elke minuut de druk van de BMP180, zodat zijn ppm klopt.
- **BMP180:** een meting met een onmogelijke temperatuur (buiten −20 tot 60 °C) of druk telt niet. Blijft het onzin, dan leest hij zijn kalibratie opnieuw in. Een oude waarde wordt na 5 s niet meer doorgegeven, anders zou een lek onopgemerkt blijven. `bmp` via USB laat temperatuur en druk zien.
- **OLED:** het gele deel van het paneel is staand een strook van 16 pixels aan de zijkant; daar komen alarmen en de status. Staat alles op zijn kop, zet `ROTATION` in `src/display.cpp` op 3. Het scherm wordt alleen opnieuw verstuurd als het beeld veranderd is, om de bus vrij te houden. De 0.91" van de inkooplijst was kapot (antwoordde op zijn adres maar weigerde elk commando).
- **Watchdog:** hangt `loop()` langer dan 5 s (bijvoorbeeld op een vastgelopen bus), dan herstart het horloge vanzelf. Bij het opstarten worden eerst 9 klokpulsen gegeven voor het geval een module SDA nog vasthoudt.

## Hardwaretests

| Test | Wat |
|---|---|
| `pio run -e test_i2c_scan -t upload` | Scan in software en met Wire, SDA/SCL-spanning, registers lezen in software, en hoe lang SCL laag blijft (te zwakke pull-ups) |
| `pio run -e test_i2c_reliability -t upload` | Per i2c-snelheid tellen hoe vaak elk ID-register goed terugkomt |
| `pio run -e test_oled -t upload` | OLED aanzetten met i2c in software, zonder Wire |
| `pio run -e test_oled_sh1106 -t upload` | Voor een scherm met een SH1106 in plaats van een SSD1306 |
| `pio run -e test_motion -t upload` | Tremor- en activiteitsberekening met kunstmatige signalen |

## Nieuwe sensor toevoegen

1. Driverbestand in `src/drivers/` met een `begin`, (optioneel) `update` en `fill`. Snelle sensoren krijgen daarnaast een `*Sample()` die in `src/sampler.cpp` wordt aangeroepen.
2. De functies in `src/drivers/drivers.h` zetten.
3. Een regel in de lijst in `src/drivers/drivers.cpp`, met het I2C-adres, het bit in `STATUS.sensors_ok` en de sensorgroep.
