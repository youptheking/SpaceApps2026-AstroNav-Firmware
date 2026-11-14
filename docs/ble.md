# Bluetooth-protocol

Het polsstation is een BLE-peripheral. De laptop (Tauri-dashboard) verbindt ermee.

## Koppelen

- Naam: `AstroNav-XXXX` (laatste deel van het MAC-adres)
- Koppelen gaat met een vaste 6-cijferige code. De code komt uit het MAC-adres, dus elk horloge heeft zijn eigen code, maar die blijft altijd hetzelfde.
- Zolang er geen gekoppelde laptop verbonden is, staat de code op het OLED-scherm. Hij staat ook in de seriële monitor bij het opstarten.
- Na de eerste keer onthouden laptop en horloge de koppeling (bonding). Daarna verbindt de laptop zonder code.
- Alle characteristics vereisen een versleutelde, geauthenticeerde verbinding. Zonder koppeling kun je niks lezen of schrijven.

## Service

Service UUID: `6e0a0001-5a3c-4c1e-9d2a-41535452304e`

| Characteristic | UUID | Richting | Wat |
|---|---|---|---|
| telemetry | `6e0a0002-…` | read, notify | sensorpakket, standaard 1× per seconde |
| ecg | `6e0a0003-…` | notify | ECG-samples, alleen na `ecg on` |
| command | `6e0a0004-…` | write | tekstcommando's van de laptop |
| reply | `6e0a0005-…` | notify | antwoord op een commando (`ok`, `pong`, `err …`) |
| history | `6e0a0006-…` | notify | opgeslagen minuten, alleen na `history <seq>` |

(alle UUID's eindigen op `-5a3c-4c1e-9d2a-41535452304e`)

## MTU

De pakketten zijn groter dan de standaard-MTU van 23 bytes. De laptop moet om een grotere MTU vragen, minstens 64. Windows doet dat automatisch. In nRF Connect op Android: *Request MTU* (of zet dat aan bij de instellingen).

## Telemetry-pakket (versie 6)

Little-endian, 40 bytes. Zie `src/telemetry.h`.

Het pakket gaat elke `rate` ms de deur uit, en daarnaast **meteen** zodra er een alarm bij komt of verdwijnt (NASA [V2 6022], zie [alarmen.md](alarmen.md)).

| Offset | Type | Veld | Eenheid |
|---|---|---|---|
| 0 | u8 | version | 6 |
| 1 | u8 | flags | zie hieronder |
| 2 | u16 | alerts | zie hieronder |
| 4 | u32 | uptime_ms | ms |
| 8 | u32 | unix_time | s, alleen geldig met `HAS_TIME` |
| 12 | u8 | heart_rate | bpm |
| 13 | u8 | spo2 | % |
| 14 | i16 | skin_temp | 0.01 °C |
| 16 | u16 | co2 | ppm, losse meting |
| 18 | u16 | ppco2_avg | 0.01 mmHg, gemiddelde over het laatste uur (de waarde waar NASA [V2 6004] om vraagt) |
| 20 | i16 | cabin_temp | 0.01 °C |
| 22 | u16 | humidity | 0.01 % |
| 24 | u16 | lux | altijd 0, de BH1750 is tijdens de bouw geschrapt (veld blijft voor het formaat) |
| 26 | u16 | gsr | huidgeleiding in 0.01 µS (stijgt bij stress) |
| 28 | i16[3] | accel x/y/z | mg |
| 34 | u8 | activity | 0-100 |
| 35 | u8 | tremor | 0-100, trilling in de 4–12 Hz band, wordt op het horloge berekend |
| 36 | u16 | battery_mv | mV |
| 38 | u8 | status | zie hieronder |
| 39 | u8 | sim_flags | welke sensorgroepen gesimuleerd zijn, zelfde bits als `flags` |

Flags (bit): 0 hartslag/SpO2, 1 huidtemperatuur, 2 lucht (CO2/temp/vocht), 3 licht, 4 IMU, 5 GSR, 6 ECG-elektrodes zitten goed, 7 klok gezet.
Een waarde telt alleen als de bijbehorende flag aan staat.

Alerts (bit): 0 lage SpO2, 1 hoge hartslag, 2 lage hartslag, 3 koorts, 4 CO2 > 3 mmHg, 5 stress, 6 batterij bijna leeg, 7 elektrodes los, 8 CO2 > 2,5 mmHg, 9 cabineklimaat. Grenzen en bronnen: [alarmen.md](alarmen.md).

Status (bit):
- 0 gesimuleerd: minstens één sensorgroep komt uit `src/sim.cpp` in plaats van een echte sensor. Welke, staat in `sim_flags`. Laat dat in het dashboard per waarde duidelijk zien.
- 1 ademhalingsoefening loopt (zie `breathe`)
- 2 ECG-stream staat aan
- 3 buzzer staat stil na `ack`

## ECG-pakket

`[teller u8][sample i16] × 25`. Er wordt gesampled op 250 Hz, dus er komen 10 pakketten per seconde. Een sample is de spanning op de AD8232-uitgang in mV, rond 1650 in rust. De teller loopt op per pakket, zodat de laptop gemiste pakketten kan zien.

## Geschiedenis

NASA-STD-3001 Vol. 2 Rev F **[V2 6001]**: *"The system shall provide environmental and suit monitoring data in formats compatible with performing temporal trend analyses."*

Het horloge slaat elke minuut een samenvatting op in zijn flash (LittleFS, `src/history.cpp`), ook als er geen laptop verbonden is. Er passen **7 dagen** in (10 080 minuten). Is de opslag vol, dan worden de oudste minuten overschreven. De data blijft bewaard bij een herstart of als de accu leeg raakt.

Elk record heeft een `seq`-nummer dat altijd oploopt, ook na een herstart. Zo weet het dashboard wat het al heeft. Na `history wipe` begint `seq` weer bij 1.

### Ophalen

1. Het dashboard verbindt en stuurt eerst `time <unix>`, zodat nieuwe minuten een tijd krijgen.
2. `history` → reply `history <oudste> <nieuwste>`
3. `history <seq>` → het horloge stuurt alle minuten vanaf `seq` via de history-characteristic. Na afloop komt er `history klaar <aantal>` op reply.
4. Het dashboard onthoudt de hoogste `seq` die het binnen heeft, en vraagt de volgende keer vanaf `seq + 1`.

Pakket: `[aantal u8][record × aantal]`. Er passen zoveel records in als de MTU toelaat, maximaal 6. Er gaat één pakket per 20 ms, dus 7 dagen ophalen duurt een halve minuut. Mist het dashboard een `seq`, dan vraagt het die gewoon opnieuw op.

### Record (versie 1)

Little-endian, 58 bytes. Zie `HistoryRecord` in `src/history.h`.

| Offset | Type | Veld | Eenheid |
|---|---|---|---|
| 0 | u8 | version | 1 |
| 1 | u32 | seq | |
| 5 | u32 | unix_time | begin van de minuut, 0 = klok was niet gezet |
| 9 | u8 | flags | welke sensoren deze minuut gemeten hebben (zelfde bits als telemetrie) |
| 10 | u8 | status | zelfde bits als telemetrie, o.a. gesimuleerd |
| 11 | u16 | alerts | alle alarmen die deze minuut aan zijn geweest |
| 13 | u8 | hr_avg | bpm |
| 14 | u8 | hr_min | bpm |
| 15 | u8 | hr_max | bpm |
| 16 | u8 | spo2_avg | % |
| 17 | u8 | spo2_min | % |
| 18 | i16 | skin_temp | 0.01 °C, gemiddelde |
| 20 | u16[12] | co2 | ppm, per 5 seconden (NASA Tabel 6.1-1: CO2 elke 5 s) |
| 44 | u16 | ppco2_avg | 0.01 mmHg, 1-uursgemiddelde aan het eind van de minuut ([V2 6004]) |
| 46 | i16 | cabin_temp | 0.01 °C, gemiddelde |
| 48 | u16 | humidity | 0.01 %, gemiddelde |
| 50 | u16 | lux | gemiddelde |
| 52 | u16 | gsr | huidgeleiding in 0.01 µS, gemiddelde |
| 54 | u8 | activity | 0-100, gemiddelde |
| 55 | u8 | tremor | 0-100, hoogste waarde |
| 56 | u16 | battery_mv | mV, laatste meting |

Een waarde is alleen geldig als de bijbehorende flag aan staat.

Met deze opslag dekken we de meetfrequenties uit NASA Tabel 6.1-1 (*Example Sampling Rates for Trending Environmental Data*): CO2 elke 5 seconden, temperatuur en luchtvochtigheid elke minuut.

**Let op:** minuten van vóór het eerste `time`-commando na een herstart hebben `unix_time = 0`. Het dashboard kan die terugrekenen vanaf de eerste minuut met een tijd, want de `seq`-nummers lopen per minuut door.

## Simulatie

De simulatie werkt **per sensor**. Bij het opstarten kijkt het horloge welke sensoren er op de I2C-bus zitten (`sensors` via USB laat het zien). Gevonden sensoren geven echte data, de rest wordt aangevuld met nepdata zolang de simulatie aan staat (standaard). Zo kun je de breakouts één voor één aansluiten. De nepdata:

- hartslag rond 72, schommelt langzaam
- elke 3 minuten 20 seconden een zuurstofdip naar ~89 %, met hogere hartslag, huidgeleiding en tremor
- CO2 loopt in 10 minuten op van 1500 naar 3200 ppm (1,1–2,4 mmHg, onder de NASA-grens) en zakt dan weer terug
- een ECG-golf die past bij de gesimuleerde hartslag (met `ecg on`)

Met `sim off` blijven alleen de echte metingen over. Als alle sensoren erin zitten, zet je de standaard in `src/main.cpp` op uit.

## Commando's

Tekst, één commando per write. Werken ook via de USB-seriële poort (115200).

| Commando | Wat |
|---|---|
| `ping` | antwoordt `pong` |
| `info` | alleen via USB: naam, koppelcode en of er een laptop verbonden is |
| `msg <tekst>` | zet een instructie op het scherm, bijv. `msg Neem rust`. `msg` zonder tekst wist hem. |
| `ack` | alarmen bevestigd, buzzer stil tot er een nieuw alarm bij komt |
| `time <unix>` | zet de klok |
| `rate <ms>` | telemetry-interval, 100 – 60000 |
| `ecg on` / `ecg off` | ECG-stream aan/uit |
| `buzz` | korte piep, om de buzzer te testen |
| `history` | reply `history <oudste> <nieuwste>` |
| `history <seq>` | opgeslagen minuten versturen vanaf `seq` (alleen via Bluetooth) |
| `history stop` | versturen stoppen |
| `history wipe` | alle opgeslagen minuten wissen |
| `history dump [n]` | alleen via USB: laatste n minuten als tekst (standaard 5) |
| `reboot` | horloge herstarten |
| `sim on` / `sim off` | simulatie aan/uit |
| `sim <veld> <waarde>` | vaste waarde voor `co2` (ppm), `spo2`, `hr`, `temp` of `cabin` (°C), om alarmen te testen. `0` = weer automatisch. |
| `breathe [minuten]` | ademhalingsoefening op het scherm (4 s in, 4 s vast, 6 s uit), standaard 2 minuten, max 30. Na afloop komt `breathe klaar` op reply. |
| `breathe off` | oefening stoppen |
| `show` | alleen via USB: laatste meetwaarden en alarmen |
| `sensors` | alleen via USB: welke sensoren gevonden zijn |
