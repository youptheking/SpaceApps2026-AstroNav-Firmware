# Inkooplijst – 2 complete sets

Alle onderdelen zijn kant-en-klare breakout-bordjes die op een zelf gefreesde draagprint komen.
De prijzen komen uit zoekresultaten van eind oktober 2026 en zijn **indicatief**. Controleer prijs en voorraad op de productpagina voordat je bestelt.

> **Let op de levertijd:** de hackathon start op 14 november. Onderdelen met "2–4 weken" levertijd komen waarschijnlijk niet op tijd binnen. Bij die onderdelen staat een alternatief.

## Kernonderdelen (per set ×2)

| # | Onderdeel | Winkel | Prijs/st | Aantal | Subtotaal | Opmerking |
|---|---|---|---|---|---|---|
| 1 | [Seeed Studio XIAO ESP32-S3](https://www.amazon.nl/ESP32S3-2-4GHz-Ondersteund-batterijen-energie-effici%C3%ABntie-interface/dp/B0BYSB66S5) | Amazon | ~€14,79 | 2 | ~€29,58 | Niet op voorraad bij [TinyTronics](https://www.tinytronics.nl/en/development-boards/microcontroller-boards/with-wi-fi/seeed-studio-xiao-esp32-s3) (€9,00, 2–4 weken). Een [3-pack op Amazon](https://www.amazon.nl/Seeed-Studio-XIAO-ESP32S3-Pack/dp/B0DJ6NQFKX) kost ~€42,99 en geeft je één reserve. |
| 2 | [MAX30102 hartslag/SpO2-module](https://www.tinytronics.nl/en/sensors/body/heart-rate-sensor-and-pulse-oximeter-max30102-i2c) | TinyTronics | €5,00 | 2 | €10,00 | I2C 0x57 |
| 3 | [MCP9808 temperatuursensor](https://www.tinytronics.nl/en/sensors/temperature/mcp9808-i2c-temperaturesensor) | TinyTronics | €6,50 | 2 | €13,00 | Een TMP117 is niet te krijgen bij TinyTronics of Amazon.nl. De MCP9808 (±0,25 °C typisch) is het alternatief uit het dossier. I2C 0x18. |
| 4 | [MPU-6050 IMU-module](https://www.tinytronics.nl/en/sensors/acceleration-rotation/mpu-6050-accelerometer-and-gyroscope-3-axis-module-3.3v-5v) | TinyTronics | €8,75 | 2 | €17,50 | I2C 0x68, 200+ op voorraad |
| 5 | [AD8232 ECG-kit](https://www.tinytronics.nl/en/sensors/body/ecg-kit-ad8232) | TinyTronics | €9,50 | 2 | €19,00 | Inclusief kabel en 3 elektrodes |
| 6 | [Grove GSR-sensor](https://www.amazon.nl/seeed-studio-101020052-Grove-sensor/dp/B078YKFS7J) | Amazon | ~€15,60 | 2 | ~€31,20 | Niet bij TinyTronics. Prijs onzeker. Analoog naar ADC. |
| 7 | [BH1750 lichtsensor](https://www.tinytronics.nl/en/sensors/optical/light-and-color/bh1750-16bit-digital-i2c-light-sensor-module) | TinyTronics | €3,50 | 2 | €7,00 | I2C 0x23 |
| 8 | [GY-SCD40 CO2-module](https://www.tinytronics.nl/en/sensors/air/humidity/gy-scd40-module-co2-humidity-temperature-sensor-i2c) | TinyTronics | €18,25 | 2 | €36,50 | Echte CO2 (NDIR/fotoakoestisch), meet ook temperatuur en luchtvochtigheid. I2C 0x62, 100+ op voorraad. |
| 9 | [0.96" OLED 128×64 I2C (wit)](https://www.tinytronics.nl/en/displays/oled/0.96-inch-oled-display-128*64-pixels-white-i2c) | TinyTronics | ~€7,00 | 2 | ~€14,00 | SSD1306, I2C 0x3C |
| 10 | [Piezobuzzer 12 mm met draden](https://www.tinytronics.nl/en/arduino/piezo-electronic-buzzer-with-soldered-wires-12mm) | TinyTronics | €0,30 | 3 | €0,90 | Aansturen met PWM. Klein genoeg voor in het horloge. |
| 11 | [PKCELL LiPo 3,7 V 500 mAh JST-PH](https://www.tinytronics.nl/en/power/batteries/li-po/pkcell-li-po-battery-3-7v-500mah-jst-ph-lp503035) | TinyTronics | €4,75 | 2 | €9,50 | Ca. 38×31×5 mm. **Levertijd 2–4 weken.** Kies anders de [350 mAh-versie](https://www.tinytronics.nl/en/power/batteries/li-po/pkcell-li-po-battery-3.7v-350mah-jst-ph-lp552035) als die wel op voorraad is. Wordt geladen door de lader op de XIAO. |
| 12 | [BINLUN klittenband-horlogeband 20/22 mm](https://www.amazon.nl/BINLUN-Horlogeband-Waterdichte-Vervangende-Klittenband/dp/B08BZM2S8G) | Amazon | ~€18,59 | 2 | ~€37,18 | Of los klittenband van de rol, als je de bevestiging zelf in de behuizing print |

**Kernonderdelen totaal: ~€225**

## Overig

| Onderdeel | Winkel | Prijs | Opmerking |
|---|---|---|---|
| [Dubbelzijdig koperplaat FR4 1,5 mm, 5× 100×70 mm](https://www.amazon.nl/Aexit-Koperen-Laminaat-Universele-Printplaat/dp/B079WHYMF7) | Amazon | ~€10 | Alleen nodig als je geen voorraad meer hebt voor de CNC |
| [Wegwerp-ECG-elektrodes (50 st)](https://www.tinytronics.nl/en/disposable-ecg-electrode) | TinyTronics | – | Reserve voor het testen van de AD8232 |
| Mini-schuifschakelaar (aan/uit tussen LiPo en XIAO) | TinyTronics | ~€0,50 | |
| Pinheaders (male/female, 2,54 mm), breadboard, jumperdraadjes | TinyTronics | – | Waarschijnlijk heb je dit al liggen. Nodig voor het breadboardprototype van set 1. |

## I2C-bus

Alle adressen zijn uniek, dus alle I2C-sensoren passen samen op één bus:

| Adres | Onderdeel |
|---|---|
| 0x18 | MCP9808 |
| 0x23 | BH1750 |
| 0x3C | OLED |
| 0x57 | MAX30102 |
| 0x62 | SCD40 |
| 0x68 | MPU-6050 |

Pinnen op de XIAO ESP32-S3: SDA en SCL, 2× ADC (GSR en ECG-uitgang), 2× digitaal (AD8232 LO+ en LO−) en 1× PWM (buzzer). Dat zijn er 7 van de 11 beschikbare.

## Afwijkingen van het projectdossier

- **TMP117 → MCP9808:** de TMP117 is niet als breakout te krijgen bij TinyTronics of Amazon.nl.
- **ICM45686 → MPU-6050:** van de ICM45686 zijn breakouts nauwelijks verkrijgbaar.
- **MAX86141 → MAX30102:** er zijn geen gangbare MAX86141-breakouts.
- **SGP30 niet gekozen:** die geeft alleen een geschatte eCO2, de SCD40 meet echte CO2.
- **TSL2591 → BH1750:** goedkoper. Geen van beide meet specifiek blauw licht. Daarvoor zou een AS7341 (spectraal) nodig zijn.
- **Toegevoegd:** LiPo-accu en aan/uit-schakelaar voor draadloze werking.
