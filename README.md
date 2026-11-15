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
- **Bluetooth Low Energy** volgens [BLE-protocol v1](docs/ble-protocol.md): ruwe streams, vitale waarden, lucht, cabine, status en alarmen. Geen koppelen op bluetooth-niveau: de app ontgrendelt het horloge met de code die op het scherm staat (`control` 0x13), zodat het op Windows, Android en met `btleplug` werkt.
- **Geschiedenis:** 7 dagen aan minuutsamenvattingen in de flash, op te halen door het dashboard
- **Robuust:** ontbrekende sensoren worden elke 10 s opnieuw gezocht, losse contacten leggen de bus niet plat, en een watchdog herstart het horloge als het toch vastloopt

## Wat er niet in zit

Gepland, maar niet in deze versie. Eerlijk vermeld, zodat niemand het in de data zoekt.

**Demomodus.** Voor de presentatie vult het horloge wat ontbreekt met nepwaarden (licht, accu, en een sensor die wegvalt), zodat het dashboard het hele plaatje kan laten zien. Dat is nooit stiekem: elk STATUS-pakket krijgt een SIM-pakket mee dat precies zegt welke waarden gesimuleerd zijn, het dashboard zet daar "SIMULATED" bij, en op het horloge staat een `*` achter de waarde. Echte sensoren en de ruwe streams (ECG, PPG, IMU, GSR) zijn altijd echt. Uit te zetten met `DEMO_MODE` in `include/pins.h`, `sim off` via usb, of `control` 0x12. Zie [docs/ble-protocol.md](docs/ble-protocol.md#uitbreidingen-op-v1).

| Wat | Waarom niet | Gevolg |
|---|---|---|
| **Lichtsensor (BH1750)** voor het circadiaans ritme | De pinvolgorde van de footprint klopte niet, herstellen kostte te veel tijd | Niet gemeten. In de demomodus een **gesimuleerde** waarde, gemarkeerd in het SIM-pakket. |
| **Accu en aan/uit-schakelaar** | Connector en schakelaar zijn tijdens de bouw van de PCB afgebroken | Het horloge draait altijd op usb, het accualarm (code 9) gaat nooit af. In de demomodus een **gesimuleerde** accu, gemarkeerd in het SIM-pakket. Weer aan te zetten met `HAS_BATTERY` in `include/pins.h`. |
| **Echte huidtemperatuur (MCP9808/TMP117)** | Zit niet op de PCB | De chiptemperatuur van de MAX30102 dient als trend, geen koortsmeting. Geen koortsalarm. |
| **ppO2** (zuurstof in de cabine) | Geen sensor | [V2 6022] noemt het, wij dekken het niet |
| **Stroomverbruik meten** en zuinig meten met de SCD41 (single-shot) | Geen tijd; zonder accu ook minder belangrijk | De SCD41 meet continu elke 5 s |
| **Wrist-raise** (scherm aan als je je pols optilt, via de INT van de MPU-6050) | INT is niet aangesloten | Het scherm staat altijd aan |
| **1-uursgemiddelde CO2 na een herstart** | Begint dan opnieuw bij nul | Na een herstart duurt het tot een uur voordat het gemiddelde weer klopt. De minuten staan wel in de geschiedenis. |
| **PTT** (bloeddruktrend), slaapanalyse, HRV-trends | Dat doet het dashboard, niet de pols | De firmware levert de ruwe ECG, PPG en RR met dezelfde klok, zodat het later kan |

**Hardware die anders is dan het ontwerp:** de footprint van de MAX30102 klopte niet (aangesloten met draadjes), R5/R6 moesten er toch op (4,3 kΩ pull-ups, zie [docs/sensoren.md](docs/sensoren.md)), en de 0.91" OLED is vervangen door een 0.96" die 90° gedraaid zit.

**Nog niet getest:** hartslag en SpO2 met een vinger tegen de sensor naast een echte saturatiemeter, de ECG met elektrodes op de borst, de dubbeltik in de behuizing, en de hele keten met het dashboard.

## NASA-bronnen

De lokale alarmen volgen **NASA-STD-3001 Volume 2, Revision F**:

- **[V2 6004]** CO2: het 1-uursgemiddelde van ppCO2 mag niet boven 3 mmHg komen
- **[V2 6022]** de crew lokaal (OLED en buzzer) én op afstand (Bluetooth naar het dashboard) waarschuwen, ook bij een drukdaling (mogelijk lek)
- **[V2 6012]** grenzen voor cabinetemperatuur en luchtvochtigheid
- **[V2 6001]** data bewaren voor trendanalyse: het horloge slaat 7 dagen aan minuutsamenvattingen op, met CO2 per 5 seconden volgens Tabel 6.1-1 (zie [de geschiedenis](docs/ble-protocol.md#uitbreidingen-op-v1))

Alle grenswaarden, hoe we de norm toepassen en de links naar de bronnen staan in [docs/alarmen.md](docs/alarmen.md).

Daarnaast gebruiken we de indeling van het **NASA Human Research Program**: de [5 hazards of human spaceflight](https://www.nasa.gov/hrp/hazards/) en de bijbehorende [risks](https://www.nasa.gov/hrp/risks/). Elk lichaamssysteem in het dashboard noemt zijn hazard en risico. Straling dekken we niet; daarvoor verwijzen we naar het CANS-project van de [Canadian Space Agency](https://www.asc-csa.gc.ca/eng/news/articles/2026/2026-02-26-monitoring-radiation-astronaut-health.asp). Het overzicht staat in [docs/status.md](docs/status.md).

## Documentatie

- [docs/status.md](docs/status.md): **wat werkt, in het Engels** (voor de projectpagina)
- [docs/sensoren.md](docs/sensoren.md): bedrading, adressen, wat je moet weten per sensor, en de hardwaretests
- [docs/ble-protocol.md](docs/ble-protocol.md): BLE-protocol v1, hoe de firmware het invult, de uitbreidingen en de USB-commando's
- [docs/alarmen.md](docs/alarmen.md): alarmen, grenswaarden, buzzerpatronen en NASA-bronnen
- [docs/inkooplijst.md](docs/inkooplijst.md): onderdelen
- [docs/ontwerpkeuzes.md](docs/ontwerpkeuzes.md): waarom deze sensoren, en welke concessies bewust zijn gedaan
- [docs/nasa-std-3001.md](docs/nasa-std-3001.md): de gebruikte NASA-grenswaarden met bronnen
- [docs/stroombudget.md](docs/stroombudget.md): stroomverbruik, accuduur en opladen
- [docs/ecg-elektrodes.md](docs/ecg-elektrodes.md): waar de ECG-elektrodes op de borst gaan
- [docs/todo.md](docs/todo.md): checklist voor de inzending

## Gebruik van AI (SI)

Volgens de regels van NASA Space Apps vermelden we waar AI-tools ("Super Intelligence", SI) zijn gebruikt.

- **Idee en concept:** eigen werk. Het concept met polsstation en dashboard, en de keuze voor de challenge, stonden vast vóór er AI werd gebruikt.
- **Voorbereiding met Claude (Anthropic):** onderzoek naar leverbare onderdelen, de stroom- en pinberekeningen, het opzoeken van de grenswaarden in NASA-STD-3001, de planning voor het weekend (`docs/todo.md`) en de eerste versies van de documenten in `docs/`. De ontwerpbeslissingen zijn door de maker genomen en gecontroleerd.
- **Code:** waar code met AI-hulp is geschreven, staat dat in de commitbeschrijving en in een opmerking in het betreffende bestand.

