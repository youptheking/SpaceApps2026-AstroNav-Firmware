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

(alle UUID's eindigen op `-5a3c-4c1e-9d2a-41535452304e`)

## Telemetry-pakket (versie 1)

Little-endian, 35 bytes. Zie `src/telemetry.h`.

| Offset | Type | Veld | Eenheid |
|---|---|---|---|
| 0 | u8 | version | 1 |
| 1 | u8 | flags | zie hieronder |
| 2 | u16 | alerts | zie hieronder |
| 4 | u32 | uptime_ms | ms |
| 8 | u32 | unix_time | s, alleen geldig met `HAS_TIME` |
| 12 | u8 | heart_rate | bpm |
| 13 | u8 | spo2 | % |
| 14 | i16 | skin_temp | 0.01 °C |
| 16 | u16 | co2 | ppm |
| 18 | i16 | cabin_temp | 0.01 °C |
| 20 | u16 | humidity | 0.01 % |
| 22 | u16 | lux | lux |
| 24 | u16 | gsr | mV |
| 26 | i16[3] | accel x/y/z | mg |
| 32 | u8 | activity | 0-100 |
| 33 | u16 | battery_mv | mV |

Flags (bit): 0 hartslag/SpO2, 1 huidtemperatuur, 2 lucht (CO2/temp/vocht), 3 licht, 4 IMU, 5 GSR, 6 ECG-elektrodes zitten goed, 7 klok gezet.
Een waarde telt alleen als de bijbehorende flag aan staat.

Alerts (bit): 0 lage SpO2, 1 hoge hartslag, 2 lage hartslag, 3 koorts, 4 hoge CO2, 5 stress, 6 batterij bijna leeg, 7 elektrodes los.

## ECG-pakket

`[teller u8][sample i16] × n`, met maximaal 100 samples per pakket. De teller loopt op per pakket, zodat de laptop gemiste pakketten kan zien.

## Commando's

Tekst, één commando per write. Werken ook via de USB-seriële poort (115200).

| Commando | Wat |
|---|---|
| `ping` | antwoordt `pong` |
| `info` | alleen via USB: naam, koppelcode en of er een laptop verbonden is |
| `msg <tekst>` | zet een instructie op het scherm, bijv. `msg Neem rust`. `msg` zonder tekst wist hem. |
| `ack` | alarm bevestigd, buzzer stil |
| `time <unix>` | zet de klok |
| `rate <ms>` | telemetry-interval, 100 – 60000 |
| `ecg on` / `ecg off` | ECG-stream aan/uit |
| `buzz` | korte piep, om de buzzer te testen |
