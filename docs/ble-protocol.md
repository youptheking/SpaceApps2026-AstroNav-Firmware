# BLE-protocol v1 – polsstation → dashboard

Status: **voorstel**, 14 november 2026. Na akkoord is dit de afspraak waar firmware en dashboard allebei op bouwen. Achtergrond en keuzes: [ble-data.md](ble-data.md).

## Algemene afspraken

- **Binair**, **little-endian** (ESP32 en x86/ARM-laptops zijn allebei little-endian), structs zonder opvulling (`__attribute__((packed))`).
- **Elke meting heeft zijn eigen pakkettype en interval.** Snelle signalen gaan in batches van ~100 ms.
- **Kommagetallen worden als geheel getal verstuurd, met een schaalfactor.** Bijvoorbeeld `temp_c100 = 3652` betekent 36,52 °C.
- **Ongeldig of onbekend:** `0` bij velden zonder fysieke nul (hartslag, SpO2), anders de maximale waarde van het type (`0xFF`, `0xFFFF`, `0x7FFF`).
- **MTU:** het dashboard vraagt bij het verbinden om **247**. Het grootste pakket is 88 bytes, dus met de standaard-MTU van 23 werkt het niet.
- **Tijd:** `t_ms` = milliseconden sinds de start van het polsstation. Het dashboard koppelt dit bij het verbinden aan de echte tijd.
- **Volgnummer:** `seq` telt per pakkettype op (en loopt rond na 65535). Een gat betekent dat er pakketten zijn verloren.

## BLE-service

| | UUID | Eigenschappen | Inhoud |
|---|---|---|---|
| Service "AstroNav" | `2075864c-7ef8-4db3-9901-ff4606218bb1` | – | – |
| `data` | `8b4d589b-2465-4b5e-9a2c-219eb307cabe` | notify | Alle meetpakketten (type 0x01–0x0A) |
| `events` | `631cd0ae-17e9-48b0-beec-431b6a4ca732` | indicate | Alarmen en gebeurtenissen (type 0x20). Indicate in plaats van notify, zodat de ontvangst wordt bevestigd en een alarm niet verloren gaat. |
| `control` | `4419bc3d-3bfd-40d1-8da1-6bcae37219b5` | write | Commando's van het dashboard (optioneel, zie onderaan) |

Apparaatnaam in de advertentie: `AstroNav-Wrist`.

## Kop (elk pakket, 8 bytes)

| Offset | Veld | Type | Betekenis |
|---|---|---|---|
| 0 | `type` | u8 | Pakkettype (zie hieronder) |
| 1 | `version` | u8 | Protocolversie, nu `1` |
| 2 | `seq` | u16 | Volgnummer per type |
| 4 | `t_ms` | u32 | Tijdstip van de **eerste sample** in het pakket, in ms sinds de start |

## Pakkettypes

| Type | Naam | Interval | Bytes | Bytes/s |
|---|---|---|---|---|
| 0x01 | `ECG` | 100 ms | 58 | 580 |
| 0x02 | `PPG` | 100 ms | 88 | 880 |
| 0x03 | `IMU` | 100 ms | 68 | 680 |
| 0x04 | `GSR` | 1 s | 28 | 28 |
| 0x05 | `VITALS` | 1 s | 20 | 20 |
| 0x06 | `RR` | 1 s (alleen als er slagen waren) | 9–19 | ~15 |
| 0x07 | `AIR` | bij elke SCD41-meting (5–60 s) | 21 | < 5 |
| 0x08 | `CABIN` | 1 s | 20 | 20 |
| 0x09 | `STATUS` | 10 s + bij verandering | 16 | ~2 |
| 0x20 | `EVENT` | bij een gebeurtenis | 14 | – |
| | **Totaal** | | | **~2,2 kB/s** |

### 0x01 `ECG` – AD8232, 250 Hz

| Veld | Type | Schaal | Betekenis |
|---|---|---|---|
| kop | 8 bytes | | |
| `samples[25]` | u16 × 25 | mV | `analogReadMilliVolts(D0)`. Sample *i* hoort bij `t_ms + i × 4 ms`. |

### 0x02 `PPG` – MAX30102, 100 Hz

| Veld | Type | Schaal | Betekenis |
|---|---|---|---|
| kop | 8 bytes | | |
| `red[10]` | u32 × 10 | ruw (18-bit) | Rood. Sample *i* hoort bij `t_ms + i × 10 ms`. |
| `ir[10]` | u32 × 10 | ruw (18-bit) | Infrarood |

### 0x03 `IMU` – MPU-6050, 50 Hz

| Veld | Type | Schaal | Betekenis |
|---|---|---|---|
| kop | 8 bytes | | |
| `samples[5]` | 5 × {i16 ax, ay, az, gx, gy, gz} | ruwe tellingen | Bereik **±4 g** (8192 LSB/g) en **±500 °/s** (65,5 LSB/(°/s)). Sample *i* hoort bij `t_ms + i × 20 ms`. |

### 0x04 `GSR` – 10 Hz

| Veld | Type | Schaal | Betekenis |
|---|---|---|---|
| kop | 8 bytes | | |
| `samples[10]` | u16 × 10 | nS (nanosiemens) | Geleiding, berekend uit de spanningsdeler. Sample *i* hoort bij `t_ms + i × 100 ms`. |

### 0x05 `VITALS` – berekend op de pols, 1 Hz

| Veld | Type | Schaal | Betekenis |
|---|---|---|---|
| kop | 8 bytes | | |
| `hr_ppg` | u8 | bpm | Hartslag uit de PPG (0 = ongeldig) |
| `hr_ecg` | u8 | bpm | Hartslag uit de ECG (0 = ongeldig) |
| `spo2` | u8 | % | SpO2 (0 = ongeldig) |
| `perfusion_c10` | u8 | ×10 % | Perfusie-index |
| `skin_temp_c100` | i16 | ×100 °C | Chiptemperatuur van de MAX30102 (trend, ~elke 10 s ververst) |
| `activity` | u8 | – | 0 = rust, 1 = licht, 2 = intensief |
| `tremor_hz_c10` | u8 | ×10 Hz | Dominante tremorfrequentie (0 = geen tremor) |
| `tremor_amp` | u8 | 0–255 | Relatieve tremorsterkte |
| `flags` | u8 | bits | bit0 huidcontact OK · bit1 bewegingsartefact · bit2 ECG-elektrode los · bit3 schok in de laatste seconde |
| `reserved` | u16 | | Reserve (0) |

### 0x06 `RR` – RR-intervallen uit de ECG

| Veld | Type | Schaal | Betekenis |
|---|---|---|---|
| kop | 8 bytes | | `t_ms` = tijdstip van de eerste R-piek in dit pakket |
| `count` | u8 | | Aantal intervallen (0–5) |
| `rr_ms[count]` | u16 × count | ms | Tijd tussen opeenvolgende R-pieken |

HRV (RMSSD, SDNN) en onregelmatig ritme berekent het dashboard hieruit.

### 0x07 `AIR` – SCD41, bij elke meting

| Veld | Type | Schaal | Betekenis |
|---|---|---|---|
| kop | 8 bytes | | |
| `co2_ppm` | u16 | ppm | CO2-concentratie |
| `temp_c100` | i16 | ×100 °C | Temperatuur (beïnvloed door de pols) |
| `rh_c100` | u16 | ×100 % | Relatieve luchtvochtigheid |
| `ppco2_c100` | u16 | ×100 mmHg | Partiële druk van CO2 (met de cabinedruk van de BMP180) |
| `ppco2_1h_c100` | u16 | ×100 mmHg | Voortschrijdend 1-uursgemiddelde |
| `nasa_status` | u8 | – | 0 = OK · 1 = boven het lange-termijndoel (2,5 mmHg) · 2 = boven de grens van **NASA-STD-3001 V2 6004** (3 mmHg, 1-uursgemiddelde) |
| `reserved` | u16 | | Reserve (0) |

### 0x08 `CABIN` – BMP180 + BH1750, 1 Hz

| Veld | Type | Schaal | Betekenis |
|---|---|---|---|
| kop | 8 bytes | | |
| `pressure_pa` | u32 | Pa | Cabinedruk (`0xFFFFFFFF` = geen BMP180) |
| `pressure_trend` | i16 | Pa/min | Verandering van de druk, voor lekdetectie |
| `lux_c10` | u32 | ×10 lux | Lichtsterkte. **BH1750 geschrapt: altijd `0xFFFFFFFF`** (veld blijft staan, zodat het protocol niet verandert). |
| `reserved` | u16 | | Reserve (0) |

### 0x09 `STATUS` – 10 s en bij verandering

| Veld | Type | Schaal | Betekenis |
|---|---|---|---|
| kop | 8 bytes | | |
| `vbat_mv` | u16 | mV | Accuspanning |
| `battery_pct` | u8 | % | Geschat percentage |
| `sensors_ok` | u8 | bits | bit0 MAX30102 · bit1 AD8232 · bit2 MPU-6050 · bit3 SCD41 · bit4 BMP180 · bit5 BH1750 · bit6 OLED · bit7 GSR |
| `alarms` | u16 | bits | Actieve alarmen, zie de lijst met codes (bit *n* = code *n*) |
| `fw_version` | u8 × 2 | | Major, minor |

### 0x20 `EVENT` – via de characteristic `events`

| Veld | Type | Schaal | Betekenis |
|---|---|---|---|
| kop | 8 bytes | | |
| `code` | u8 | | Zie de tabel hieronder |
| `state` | u8 | | 1 = gestart, 0 = opgeheven |
| `severity` | u8 | | 0 = info · 1 = waarschuwing · 2 = alarm |
| `value` | i16 | afhankelijk van de code | De waarde die de gebeurtenis veroorzaakte |
| `source` | u8 | | 0 = gedetecteerd op de pols · 1 = verstuurd door het dashboard (`control` 0x04) · 2 = bevestigd door de astronaut |

| Code | Gebeurtenis | `value` | Ernst |
|---|---|---|---|
| 0 | CO2 boven de NASA-grens (1-uursgemiddelde > 3 mmHg) | ppCO2 ×100 | alarm |
| 1 | CO2 actueel boven 3 mmHg (vroege waarschuwing) | ppCO2 ×100 | waarschuwing |
| 2 | SpO2 laag | % | alarm |
| 3 | Hartslag hoog | bpm | waarschuwing |
| 4 | Hartslag laag | bpm | waarschuwing |
| 5 | ECG-elektrode los | – | info |
| 6 | Geen huidcontact (PPG) | – | info |
| 7 | Schok gedetecteerd | piek in mg | waarschuwing |
| 8 | Drukdaling (mogelijk lek) | Pa/min | alarm |
| 9 | Accu bijna leeg | % | waarschuwing |
| 10 | Hoge stress (HRV/GSR) | – | waarschuwing |
| 11 | Melding bevestigd door de astronaut (dubbeltik) | code van de bevestigde melding | info |

## Grenswaarden en bronnen

**NASA-bronnen in dit protocol:**

| Waar | Bron | Wat |
|---|---|---|
| `AIR`: `ppco2_c100`, `ppco2_1h_c100`, `nasa_status` · `EVENT` code 0 en 1 | **NASA-STD-3001 Vol. 2, V2 6004** + NASA OCHMO Technical Brief CO2 | CO2 als partiële druk, grens 3 mmHg (1-uursgemiddelde), doel 2,5 mmHg |
| `CABIN`: `pressure_pa`, `pressure_trend` · `EVENT` code 8 | **NASA-STD-3001 Vol. 2, V2 6022** | De bemanning **lokaal én op afstand** waarschuwen als cabinedruk, luchtvochtigheid, temperatuur, ppO2 of ppCO2 buiten veilige grenzen komen. Daarom zijn er een druktrend en een lekalarm. |
| `events` + buzzer/OLED | **NASA-STD-3001 Vol. 2, V2 6022** | De architectuur: lokaal alarm op de pols plus een alarm op afstand in het dashboard |
| Stressmeting (HRV, GSR) | NASA OCHMO Technical Brief Behavioral Health | Onderbouwt **waarom** stress bewaakt moet worden (isolatie). Geeft geen grenswaarden. |

**Fysiologische grenzen (geen NASA-bron):** in de NASA-documenten die zijn doorzocht staan **geen alarmwaarden** voor SpO2, hartslag of stress. Die komen daarom uit algemene medische conventies en worden **als zodanig vermeld**. Alle waarden zijn instelbaar, en het systeem geeft **adviezen, geen diagnoses**.

| Code | Gebeurtenis | Voorstel | Bron | Zekerheid |
|---|---|---|---|---|
| 2 | SpO2 laag | **< 94 %** gedurende 30 s = waarschuwing · **< 90 %** gedurende 30 s = alarm | WHO Pulse Oximetry Training Manual (94 % en 90 % als drempels) | Gemiddeld. **Nalezen in het WHO-document.** |
| 3 | Hartslag hoog | **> 120 bpm in rust** (IMU: activiteit = 0) gedurende 5 min | Eigen ontwerpkeuze, op basis van een normale rusthartslag van 60–100 bpm (bijv. American Heart Association) | Ontwerpkeuze. Tijdens training is een hoge hartslag normaal, daarom gekoppeld aan de IMU. |
| 4 | Hartslag laag | **< 40 bpm** gedurende 1 min | Eigen ontwerpkeuze. Onder 60 heet bradycardie, maar bij getrainde mensen (astronauten) is 40–60 normaal. | Ontwerpkeuze |
| 10 | Hoge stress | HRV (RMSSD) **> 30 % onder de eigen baseline** gedurende 10 min, plus verhoogde GSR | Eigen ontwerpkeuze. Er bestaat geen universele grens voor HRV, alleen een persoonlijke vergelijking is zinvol. | Ontwerpkeuze. Advies ("Neem rust", "Start ademhalingstraining"), geen alarm. |

**Principe: vergelijken met de eigen baseline.** Voor HRV, temperatuur en GSR geldt geen vaste grens voor iedereen. Het systeem leert in de eerste periode de normale waarden van de astronaut, en signaleert daarna afwijkingen daarvan. Zo werken commerciële wearables ook.

## `control` – commando's van het dashboard

Eén byte commando plus parameters. Schrijven met **write-with-response**: dan bevestigt de BLE-laag dat de pols het commando heeft ontvangen.

| Commando | Parameters | Betekenis |
|---|---|---|
| 0x01 | – | Buzzer testen |
| 0x02 | u8 0/1 | Ruwe streams (ECG/PPG/IMU/GSR) uit of aan, om stroom te sparen |
| 0x03 | u8 code | Melding afgehandeld in het dashboard: pols stopt met piepen voor deze code |
| **0x04** | u8 code · u8 severity · u8 message_id | **Melding van het dashboard naar de pols.** Voor alles wat alleen het dashboard kan zien, zoals trends over langere tijd (stress via HRV, slaap, licht). De pols toont de tekst op het OLED en speelt het buzzerpatroon bij de ernst. Daarna stuurt hij een `EVENT` terug met dezelfde code, `state` = 1 en `source` = 1, als bevestiging dat de melding is getoond. |

### Melding van het dashboard naar de pols (0x04)

**Waarom:** sommige signalen ziet alleen het dashboard, omdat het trends over langere tijd analyseert. De astronaut moet die ook voelen of horen, niet alleen op de laptop lezen. Dit volgt ook **NASA-STD-3001 V2 6022**: lokaal én op afstand waarschuwen.

**Buzzerpatronen per ernst** (zo hoor je het verschil zonder te kijken):

| `severity` | Patroon | Gebruik |
|---|---|---|
| 0 = info | Eén korte piep | Advies zonder haast |
| 1 = waarschuwing | 3× kort, elke 30 s herhaald tot bevestiging | Stress, hartslag, CO2-vroegwaarschuwing |
| 2 = alarm | Continu afwisselend hoog/laag tot bevestiging | CO2 boven de NASA-grens, SpO2 laag, drukdaling |

**OLED-teksten** (`message_id`, vast in de firmware, 2 regels op 128×32):

| id | Tekst |
|---|---|
| 0 | `Neem rust` |
| 1 | `Start ademhaling` / `training 2 min` |
| 2 | `CO2 hoog` / `ventilatie check` |
| 3 | `Zuurstof laag` / `stop activiteit` |
| 4 | `Druk daalt` / `check cabine` |
| 5 | `Slaap tekort` / `plan rustmoment` |
| 6 | `Elektrode los` / `check borst` |
| 7 | `Accu laag` / `opladen` |

**Bevestigen door de astronaut:** het horloge heeft geen knop. Een **dubbeltik** op het horloge (gedetecteerd door de MPU-6050) bevestigt de melding. De pols stopt dan met piepen en stuurt `EVENT` code 11 met `source` = 2. Het dashboard ziet zo dat de astronaut de melding heeft gezien.

**Geen verbinding?** Dan komen meldingen van het dashboard niet aan. De lokale alarmen op de pols (CO2, SpO2, hartslag, elektrode los, druk) werken altijd, ook zonder laptop.

## Voorbeeld in C++ (firmware)

```cpp
#pragma pack(push, 1)
struct Header { uint8_t type, version; uint16_t seq; uint32_t t_ms; };

struct EcgPacket    { Header h; uint16_t mv[25]; };                  // 58 bytes
struct PpgPacket    { Header h; uint32_t red[10], ir[10]; };         // 88 bytes
struct ImuSample    { int16_t ax, ay, az, gx, gy, gz; };
struct ImuPacket    { Header h; ImuSample s[5]; };                   // 68 bytes
struct GsrPacket    { Header h; uint16_t nS[10]; };                  // 28 bytes
struct VitalsPacket { Header h; uint8_t hr_ppg, hr_ecg, spo2, perfusion_c10;
                      int16_t skin_temp_c100; uint8_t activity, tremor_hz_c10,
                      tremor_amp, flags; uint16_t reserved; };       // 20 bytes
struct AirPacket    { Header h; uint16_t co2_ppm; int16_t temp_c100;
                      uint16_t rh_c100, ppco2_c100, ppco2_1h_c100;
                      uint8_t nasa_status; uint16_t reserved; };     // 21 bytes
struct CabinPacket  { Header h; uint32_t pressure_pa; int16_t pressure_trend;
                      uint32_t lux_c10; uint16_t reserved; };        // 20 bytes
struct StatusPacket { Header h; uint16_t vbat_mv; uint8_t battery_pct, sensors_ok;
                      uint16_t alarms; uint8_t fw_major, fw_minor; }; // 16 bytes
struct EventPacket  { Header h; uint8_t code, state, severity; int16_t value;
                      uint8_t source; };                           // 14 bytes
#pragma pack(pop)

static_assert(sizeof(PpgPacket) == 88, "PPG-pakket heeft de verkeerde grootte");
```
Het `RR`-pakket heeft een variabele lengte: kop + `count` + `count × u16`.

## Uitlezen in Rust (dashboard)

Lees velden op vaste offsets met `u16::from_le_bytes`, `u32::from_le_bytes` enzovoort. Controleer eerst de **lengte** en de **versie** van het pakket voordat je het verwerkt. Pakketten met een onbekend type sla je over.
