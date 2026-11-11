# NASA Space Apps 2026 – Team AstroNav – Firmware (polsstation)

> **Status: nog niet gestart.** De ontwikkeling begint op zaterdag 14 november 2026 om 09:00 (NL-tijd).
> Deze repo bevat bewust alleen projectinformatie. Er staat nog geen code in.

> Dit is een **NASA Space Apps Challenge 2026**-project en staat los van de andere AstroNav-projecten (AstroNav-Nano en verwante repos).

Zusterrepo: **SpaceApps2026-AstroNav-Dashboard** (de desktopapp)

---

## Deelname

| | |
|---|---|
| Teamnaam | Team AstroNav |
| Deelnamevorm | Solo |
| Locatie inschrijving | Amsterdam (virtueel evenement) |
| Werkomgeving | Eigen werkplaats met desktop-CNC, 3D-printers, soldeerstation en testapparatuur |

## Tijdlijn

| Moment | Datum en tijd (NL) |
|---|---|
| Start | Zaterdag 14 november 2026, 09:00 |
| Inleverdeadline | Zondag 15 november 2026, 23:59 |

## Challenge

**Create Health Monitoring Software for Astronauts on Space Missions**

Langdurige ruimtemissies brengen risico's met zich mee: straling, isolatie, gewichtloosheid, veranderingen in het immuunsysteem, spieratrofie, belasting van hart en bloedvaten, en stress. Astronauten moeten hun eigen gezondheid kunnen bewaken en direct kunnen ingrijpen.

## Oplossing

Een samenhangend systeem met twee delen:

1. **Polsstation (deze repo):** een sensorstation dat je om de pols draagt, met een gefreesde PCB en een 3D-geprinte behuizing. Het meet lichaam en cabine, geeft lokaal waarschuwingen en stuurt telemetrie via Bluetooth Low Energy.
2. **Desktopdashboard:** een laptopapp die de data ontvangt en realtime grafieken, trends en een beslissingspaneel toont.

## Hardware en vormfactor

- **Vormfactor:** ergonomisch polshorloge met klittenband
- **PCB:** dubbelzijdig, gefreesd op de desktop-CNC
- **Behuizing:** 3D-geprint (PETG of PLA) met ventilatieroosters en koperen GSR-contactvlakken aan de onderzijde
- **Hoofdcontroller:** ESP32 met ingebouwde BLE
- **Lokale indicatie:**
  - 0.96" I2C OLED: actuele status en directe instructies, zoals "Neem rust", "Start ademhalingstraining" of "Verhoogde CO2"
  - Piezobuzzer: geluidswaarschuwing bij acute alarmsituaties

## Sensoren

| Component | Koppeling | Positie | Functie en relevantie voor ruimtevaart |
|---|---|---|---|
| ESP32 | Systeemcontroller | Intern op PCB | Leest sensoren uit, stuurt OLED en buzzer aan, verzorgt BLE-streaming |
| MAX30102 / MAX86141 | I2C | Onderzijde (tegen huid) | Optische meting van hartslag en zuurstofsaturatie (SpO2) |
| TMP117 / MCP9808 | I2C | Onderzijde (tegen huid) | Nauwkeurige lichaamstemperatuur (immuunreactie, koorts) |
| ICM45686 / MPU6050 | I2C of SPI | Intern op PCB | 3-assige IMU: activiteit, training, slaapkwaliteit, spiertremoren |
| GSR-contacten | Analoog (ADC) | Onderzijde (tegen huid) | Twee gefreesde koperen vlakken met een spanningsdeler, voor huidgeleiding (acute stress) |
| AD8232 | Analoog (ADC) | Intern, kabel naar de borst | Continue ECG-hartgolf via 3 plakelektrodes op de borst. Leads-off-detectie meldt een losse elektrode. |
| TSL2591 / BH1750 | I2C | Bovenzijde | Omgevingslicht en blauw licht voor bewaking van circadiaans ritme en slaap |
| SCD40 / SGP30 | I2C | Zijkant (met rooster) | CO2 en luchtkwaliteit in de cabine, tegen verstikking en hoofdpijn |
| 0.96" OLED | I2C | Bovenzijde | Directe actiepunten voor de astronaut |
| Piezobuzzer | GPIO (PWM) | Intern op PCB | Alarm bij kritieke daling van SpO2 of stijging van CO2 |

## Firmware (gepland)

- **Taal:** C++ op de ESP32
- **Sensoruitlezing:** alle sensoren met hoge frequentie
- **Lokale beslislogica:** waarschuwingen en instructies direct op het OLED-scherm, buzzer bij noodsituaties, ook zonder verbinding met de laptop
- **Draadloze telemetrie:** gestructureerde datapakketten via BLE naar het desktopdashboard
