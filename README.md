# NASA Space Apps 2026 – Team AstroNav – Firmware (polsstation)

> **Status: firmware 1.0 werkt op de PCB.** Alle sensoren worden uitgelezen, de lokale alarmen werken, en de data gaat via Bluetooth volgens [BLE-protocol v1](docs/ble-protocol.md).
>
> Bouwen en flashen met PlatformIO: `pio run -t upload`. Board: Seeed XIAO ESP32-S3.

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
- **Behuizing:** 3D-geprint (PETG of PLA) met ventilatieroosters en koperen contactvlakken
- **Hoofdcontroller:** Seeed XIAO ESP32-S3 met ingebouwde BLE
- **Lokale indicatie:**
  - 0.96" tweekleurige I2C OLED, staand: waarden, alarmen in de gele strook, en instructies van het dashboard zoals "Take a break" of "Start breathing exercise"
  - Piezobuzzer: een eigen patroon per ernst (info, waarschuwing, alarm)
  - Bevestigen met een **dubbeltik** op het horloge

## Sensoren

| Component | Koppeling | Positie | Functie en relevantie voor ruimtevaart |
|---|---|---|---|
| XIAO ESP32-S3 | Systeemcontroller | Intern op PCB | Leest sensoren uit, stuurt OLED en buzzer aan, verzorgt BLE-streaming |
| MAX30102 | I2C | Onderzijde (tegen huid) | PPG op 100 Hz: hartslag, zuurstofsaturatie (SpO2), perfusie. De chiptemperatuur dient als trend van de huidtemperatuur. |
| MPU-6050 | I2C | Intern op PCB | 3-assige IMU: activiteit, tremor (sterkte en frequentie), schok, dubbeltik |
| AD8232 | Analoog (ADC) | Kabel naar de borst | ECG op 250 Hz, R-pieken en RR-intervallen (HRV), elektrode-los-detectie |
| GSR-contacten | Analoog (ADC) | Onderzijde (tegen huid) | Gefreesde koperen vlakken voor huidgeleiding (acute stress) |
| SCD41 | I2C | Zijkant (met rooster) | Echte CO2, plus temperatuur en luchtvochtigheid in de cabine |
| BMP180 | I2C | Intern | Cabinedruk: ppCO2 in mmHg en een lekalarm bij een drukdaling |
| ~~BH1750~~ | – | – | Tijdens de bouw geschrapt: de pinvolgorde van de footprint klopte niet. |
| 0.96" OLED | I2C | Bovenzijde | Directe actiepunten voor de astronaut |
| Piezobuzzer | GPIO (PWM) | Intern op PCB | Lokaal alarm, ook zonder laptop |

## Firmware

- **C++ met PlatformIO** (Arduino-core voor de ESP32-S3)
- **Snelle sensoren in een eigen task** op een vaste tik van 4 ms, zodat ECG, PPG, IMU en GSR dezelfde klok hebben (nodig voor RR-intervallen en later PTT)
- **Lokale beslislogica:** alarmen met de codes van het protocol, direct op het OLED en met de buzzer, ook zonder verbinding met de laptop
- **Bluetooth Low Energy** volgens [BLE-protocol v1](docs/ble-protocol.md): ruwe streams, vitale waarden, lucht, cabine, status en alarmen. Koppelen met een vaste code die op het scherm staat.
- **Geschiedenis:** 7 dagen aan minuutsamenvattingen in de flash, op te halen door het dashboard
- **Robuust:** ontbrekende sensoren worden elke 10 s opnieuw gezocht, losse contacten leggen de bus niet plat, en een watchdog herstart het horloge als het toch vastloopt

## NASA-bronnen

De lokale alarmen volgen **NASA-STD-3001 Volume 2, Revision F**:

- **[V2 6004]** CO2: het 1-uursgemiddelde van ppCO2 mag niet boven 3 mmHg komen
- **[V2 6022]** de crew lokaal (OLED en buzzer) én op afstand (Bluetooth naar het dashboard) waarschuwen, ook bij een drukdaling (mogelijk lek)
- **[V2 6012]** grenzen voor cabinetemperatuur en luchtvochtigheid
- **[V2 6001]** data bewaren voor trendanalyse: het horloge slaat 7 dagen aan minuutsamenvattingen op, met CO2 per 5 seconden volgens Tabel 6.1-1 (zie [de geschiedenis](docs/ble-protocol.md#uitbreidingen-op-v1))

Alle grenswaarden, hoe we de norm toepassen en de links naar de bronnen staan in [docs/alarmen.md](docs/alarmen.md).

## Documentatie

- [docs/sensoren.md](docs/sensoren.md): bedrading, adressen, wat je moet weten per sensor, en de hardwaretests
- [docs/ble-protocol.md](docs/ble-protocol.md): BLE-protocol v1, hoe de firmware het invult, de uitbreidingen en de USB-commando's
- [docs/alarmen.md](docs/alarmen.md): alarmen, grenswaarden, buzzerpatronen en NASA-bronnen
- [docs/inkooplijst.md](docs/inkooplijst.md): onderdelen
