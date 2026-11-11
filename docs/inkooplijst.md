# Inkooplijst – 1 set met reserves voor kwetsbare onderdelen

Alle onderdelen zijn kant-en-klare breakout-bordjes die op een zelf gefreesde draagprint komen.
Uitgangspunten: **zo klein mogelijk voor een acceptabele prijs** (het wordt om de arm gedragen) en **eerst TinyTronics, dan Amazon**.

> **Controleer voorraad en prijs live voordat je bestelt.** De gegevens hieronder komen uit zoekmachine-snapshots van 11 november 2026, niet van de live productpagina's. Bij een paar onderdelen gaven verschillende snapshots een andere prijs of voorraad. Dat staat erbij vermeld.
>
> **Levertijd:** de hackathon start op zaterdag 14 november. Alles met "2–4 weken" valt af. Bestel uiterlijk woensdag 11 november, zodat het donderdag of vrijdag binnen is.

Resultaat: **alles kan in één bestelling bij TinyTronics.** Je koopt 1 set, met reserves alleen voor de goedkope onderdelen die stuk kunnen of problemen kunnen geven (XIAO, MAX30102, OLED). Het prototype bouw je op het breadboard, daarna gaan dezelfde modules naar de draagprint. Zet ze daar op female headers in plaats van ze vast te solderen, dan kun je ze verplaatsen en wisselen. Amazon is alleen nog nodig voor koperplaat (als je die niet meer hebt) of als een TinyTronics-onderdeel toch uitverkocht blijkt.

## Kernonderdelen

| # | Onderdeel | Winkel | Prijs/st | Aantal | Subtotaal | Afmeting | Opmerking |
|---|---|---|---|---|---|---|---|
| 1 | [Seeed Studio XIAO ESP32-S3](https://www.tinytronics.nl/en/development-boards/microcontroller-boards/with-wi-fi/seeed-studio-xiao-esp32-s3) | TinyTronics | €9,00 | 2 | €18,00 | 21×17,5 mm | 1 + 1 reserve: als de enige XIAO sneuvelt (ESD, kortsluiting), ligt het hele project stil. 200+ op voorraad (snapshot). Goedkoper dan Amazon (~€14,79). Werkt met een externe U.FL-antenne: houd ruimte vrij in de behuizing. **Niet** de Sense-versie, die heeft 2–4 weken levertijd. |
| 2 | [MAX30102 hartslag/SpO2 (MAX30102MOD)](https://www.tinytronics.nl/en/sensors/body/heart-rate-sensor-and-pulse-oximeter-max30102-i2c) | TinyTronics | €5,00 | 2 | €10,00 | klein | Op voorraad. 1 + 1 reserve, want goedkope MAX30102-modules hebben soms I2C-pull-ups naar 1,8 V die de bus verstoren. Test hem eerst los. I2C 0x57. |
| 3 | ~~MCP9808~~ **geschrapt** | – | – | – | – | – | De temperatuur komt uit de ingebouwde sensor van de MAX30102 (±1 °C, resolutie 0,0625 °C). Voor een trend ten opzichte van de baseline is dat genoeg, en het scheelt €6,50 en een bordje van ~19×13 mm. |
| 4 | [MPU-6050 IMU-module](https://www.tinytronics.nl/en/sensors/acceleration-rotation/mpu-6050-accelerometer-and-gyroscope-3-axis-module-3.3v-5v) | TinyTronics | €8,75 | 1 | €8,75 | ~21×16 mm | Geen wezenlijk kleinere IMU op voorraad. De MPU-6500 is even groot en heeft een ander WHO_AM_I, waardoor sommige MPU6050-libraries niet werken. I2C 0x68. |
| 5 | [AD8232 ECG-kit](https://www.tinytronics.nl/en/sensors/body/ecg-kit-ad8232) | TinyTronics | €9,50 | 1 | €9,50 | **35,5×30,5×10,5 mm** | **Grootste bordje van het ontwerp.** Geen kleinere AD8232-breakout gevonden. **Continue meting** met 3 plakelektrodes op de borst en de kabel van de kit naar de pols, dus de 3,5 mm-jack blijft zitten (met trekontlasting in de behuizing). Leads-off-detectie (LO+/LO−) meldt automatisch een losse elektrode. Droge elektrodes op één pols geven geen bruikbaar ECG. |
| 6 | GSR: **zelfbouw**, geen module | – | ~€0 | – | – | – | Twee gefreesde koperen vlakken plus een weerstand (spanningsdeler) naar een ADC-pin. Vervangt de Grove GSR (~€15,60 bij Amazon, 20×40 mm plus klittenband-vingerbandjes). |
| 7 | [BH1750 lichtsensor](https://www.tinytronics.nl/en/sensors/optical/light-and-color/bh1750-16bit-digital-i2c-light-sensor-module) | TinyTronics | €3,50 | 1 | €3,50 | ~19×14 mm | I2C 0x23 |
| 8 | [GY-SCD41 CO2-module](https://www.tinytronics.nl/en/sensors/air/humidity/gy-scd41-module-co2-humidity-temperature-sensor-i2c) | TinyTronics | €26,50–29,50 | 1 | ~€26,50–29,50 | sensor 10×10 mm | **Gewijzigd van SCD40.** De SCD40 is alleen gespecificeerd tot 2000 ppm, terwijl de CO2 in een ruimtestation daar vaak boven zit. De SCD41 is gespecificeerd tot 5000 ppm en heeft een single-shot-modus, wat veel stroom spaart. Snapshots: 6 st. bij de ene listing en 50+ bij de andere. Fallback, en de grootste besparingsmogelijkheid (~€10): [GY-SCD40](https://www.tinytronics.nl/en/sensors/air/humidity/gy-scd40-module-co2-humidity-temperature-sensor-i2c) (€18,25, 100+). I2C 0x62. |
| 9 | [0.91" OLED 128×32 I2C (wit)](https://www.tinytronics.nl/en/displays/oled/0.91-inch-oled-display-128*32-pixels-white-i2c) | TinyTronics | €4,00 | 2 | €8,00 | ~38×12 mm | 1 + 1 reserve (dun glas, breekt makkelijk). **Gewijzigd van 0.96".** Smal strookje dat beter op een pols past, en goedkoper. Er passen 2 regels grote tekst op ("Neem rust", "CO2 hoog"). SSD1306, I2C 0x3C. |
| 10 | [Piezobuzzer 12 mm met draden](https://www.tinytronics.nl/en/arduino/piezo-electronic-buzzer-with-soldered-wires-12mm) | TinyTronics | €0,30 | 2 | €0,60 | Ø12 mm | Aansturen met PWM |
| 11 | [LiPo 3,7 V 250 mAh](https://www.tinytronics.nl/en/power/batteries/li-po/li-po-accu-3.7v-250mah) | TinyTronics | €5,00 | 1 | €5,00 | 30,5×17,5×6,5 mm | **Gewijzigd van 500 mAh** (38×31 mm, 2–4 weken levertijd). Rekening houdend met ~70–90 mA verbruik (ESP32-S3 met BLE, SCD41 periodiek, OLED) is dat **ongeveer 3 uur**. Genoeg voor een demo, en met de single-shot-modus van de SCD41 meer. Voorraad volgens snapshots tegenstrijdig. De 150 mAh (27×20 mm) heeft 2–4 weken levertijd. De XIAO laadt via soldeerpads op de onderkant, dus de stekker gaat er toch af. **Controleer of de accu een beveiligingsprint (PCM) heeft.** De lader van de XIAO beschermt de accu waarschijnlijk niet tegen diepontlading. |
| 12 | [Goobay klittenbandrol 1 m](https://www.tinytronics.nl/en/tools-and-mounting/installation-and-mounting-material/cable-ties-zip-ties-tyraps/goobay-70360-hook-and-loop-fastener-roll-1m) | TinyTronics | €3,00 | 1 | €3,00 | – | **Vervangt de BINLUN-horlogeband (~€37).** Print lussen in de behuizing en knip de band op maat. 10+ op voorraad. |

**Kernonderdelen totaal: ~€93–96**, met elektrodes en schakelaars ~€100–102 (was ~€175–185 voor 2 sets). Met de SCD40 in plaats van de SCD41 ~€8–11 minder.

## Overig – bestellen

| Onderdeel | Winkel | Prijs | Aantal | Opmerking |
|---|---|---|---|---|
| [Wegwerp-ECG-elektrodes (50 st)](https://www.tinytronics.nl/en/sensors/body/disposable-ecg-electrode-50-pieces) | TinyTronics | €6,00 | 1 | Kleinste verpakking. De 3 uit de kit zijn te weinig voor een weekend testen en opnieuw plakken. |
| [Mini SMD-schuifschakelaar MK12C02](https://www.tinytronics.nl/en/switches/manual-switches/slide-switches/mini-smd-pcb-switch-mk12c02) | TinyTronics | €0,20 | 2 | Aan/uit tussen LiPo en XIAO. Wil je through-hole: [SWITCHPCB](https://www.tinytronics.nl/en/switches/manual-switches/slide-switches/small-switch-for-pcb-or-breadboard). |

## Overig – controleer eigen voorraad, anders meebestellen bij TinyTronics

| Onderdeel | Waarvoor |
|---|---|
| Female pinheaders 2,54 mm (2× 1×7) | De XIAO in een voetje op de draagprint, zodat hij herbruikbaar blijft en je hem bij een fout snel wisselt. Kost wel ~8 mm extra hoogte. Voor de laagste opbouw soldeer je hem direct. |
| Male pinheaders 2,54 mm | Bij de meeste breakouts zitten ze los bij. Voor de XIAO niet altijd. |
| Breadboard (full size) + male-male en male-female jumperdraadjes | Prototype van set 1 op zaterdagochtend |
| Dun siliconendraad (28–30 AWG), een paar kleuren | Bedrading in het horloge. Is flexibel en dun, beter dan jumperdraadjes. |
| Weerstanden 1 kΩ–1 MΩ (assortiment) | GSR-spanningsdeler, eventueel extra I2C-pull-ups, en **2× ~200 kΩ als spanningsdeler voor de accuspanning**. De XIAO ESP32-S3 heeft geen ingebouwde accumeting, en zonder die deler weet je niet hoe vol de accu is. |
| Condensatoren 100 nF + 10 µF | Ontkoppeling bij de SCD41 (pieken tot ~200 mA) en de XIAO |
| USB-C-**data**kabel | Flashen van de XIAO. Laadkabels zonder datalijnen zijn een klassieke tijdverspiller. |
| Dubbelzijdig koperplaat FR4 ([Amazon, ~€10](https://www.amazon.nl/Aexit-Koperen-Laminaat-Universele-Printplaat/dp/B079WHYMF7)) | Draagprint frezen. Dunner (0,8 mm) is beter voor een pols, als je dat hebt. |
| Freesjes/graveerstiften (V-bit 20–30°, 0,8–1 mm boor, 1–2 mm uitfrees) | PCB frezen |
| Soldeertin, flux, desoldeerlint | Soldeerwerk, en ook het vertinnen van de GSR-vlakken tegen oxidatie |
| Kaptontape | Isoleren van de onderkant van de breakouts en de LiPo |
| Krimpkous (dun) | Draadverbindingen |
| M2-schroefjes / inserts | Print vastzetten in de behuizing |
| PETG- of PLA-filament | Behuizing. PETG is beter tegen zweet en warmte op de huid. |

## Let op bij de bouw

- **I2C-pull-ups stapelen:** bijna elke breakout heeft eigen pull-ups (meestal 4,7–10 kΩ). Zes modules parallel geeft ~1 kΩ. Meestal werkt dat nog, maar haal bij I2C-fouten op een paar modules de pull-upweerstanden eraf.
- **Antenne:** de U.FL-antenne van de XIAO niet onder de LiPo of tegen koper leggen
- **Volgorde op zaterdag:** test elke sensor eerst apart op het breadboard (te beginnen met de MAX30102), en sluit dan pas alles samen op de bus aan.

## Bewust overwogen en niet gekozen

| Alternatief | Waarom niet |
|---|---|
| [MAX30205-module](https://www.amazon.nl/Lichaamstemperatuursensormodule-Nauwkeurigheid-Laagspanningssensorkaart-Lichaamstemperatuur-Temperatuuralarmfuncties/dp/B0D48KVGBC) (Amazon, ~€14,74) | Gemaakt voor lichaamstemperatuur (0,1 °C in het bereik 37–39 °C). Levering via Amazon, I2C 0x48. Upgrade voor een productieversie op een eigen print. |
| XIAO nRF52840 Sense (IMU ingebouwd, veel zuiniger) | Bij TinyTronics 2–4 weken levertijd. De firmware is gepland op de ESP32. |
| SCD40 | Alleen gespecificeerd tot 2000 ppm, zie #8 |
| 0.96" OLED | Breder en duurder dan de 0.91" |
| Grove GSR | Groot, duur en alleen bij Amazon. Een spanningsdeler volstaat voor een demo. |
| LSM6DSOX (Arduino Modulino) | Modulino-bordjes zijn groter dan de MPU-6050 |

## I2C-bus

Alle adressen zijn uniek, dus alle I2C-sensoren passen samen op één bus:

| Adres | Onderdeel |
|---|---|
| 0x23 | BH1750 |
| 0x3C | OLED |
| 0x57 | MAX30102 |
| 0x62 | SCD41 |
| 0x68 | MPU-6050 |

Pinnen op de XIAO ESP32-S3: SDA en SCL, 3× ADC (GSR, ECG-uitgang, accuspanning), 2× digitaal (AD8232 LO+ en LO−) en 1× PWM (buzzer). Dat zijn er 8 van de 11 beschikbare.

## Afwijkingen van het projectdossier

- **TMP117 → ingebouwde sensor van de MAX30102:** een losse temperatuursensor kost ruimte. Voor het prototype meten we alleen de trend. Op een eigen print zou dat een MAX30205 of TMP117 worden.
- **ICM45686 → MPU-6050:** van de ICM45686 zijn breakouts nauwelijks verkrijgbaar.
- **MAX86141 → MAX30102:** er zijn geen gangbare MAX86141-breakouts.
- **SGP30 / SCD40 → SCD41:** de SGP30 geeft alleen een geschatte eCO2. De SCD41 meet echte CO2 en dekt het bereik van een ruimtestation (tot 5000 ppm).
- **TSL2591 → BH1750:** kleiner en goedkoper. Geen van beide meet specifiek blauw licht. Daarvoor zou een AS7341 (spectraal) nodig zijn.
- **Grove GSR → zelfbouw-spanningsdeler:** voor formaat en levertijd.
- **ECG met plakelektrodes op de borst in plaats van koperen vlakken:** voor continue meting zonder handeling van de astronaut. De MAX30102 blijft de continue bron voor hartslag en HRV. De ECG voegt ritme- en aritmiedetectie toe.
- **0.96" → 0.91" OLED:** smaller formaat voor de pols.
- **Toegevoegd:** LiPo-accu en aan/uit-schakelaar voor draadloze werking.
