# BLE-protocol v1 – polsstation → dashboard

Status: **afgesproken en geïmplementeerd** in de firmware 1.0 (15 november 2026). Firmware en dashboard bouwen allebei hierop. De gedeelde definities staan in `include/astronav_protocol.h`. Wat de firmware precies doet en de paar uitbreidingen staan onderaan, bij [Implementatie in de firmware](#implementatie-in-de-firmware).

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

## Implementatie in de firmware

Zo is het protocol in de firmware (versie 1.0) ingevuld. Alles hierboven geldt, dit zijn de details en de keuzes die het protocol open liet.

### Status per pakket op het prototype

Wat er op het gebouwde horloge echt doorkomt. Een dashboard kan op de ongeldige waarden rekenen in plaats van op deze tabel, maar zo weet je wat je kunt verwachten.

| Type | Pakket | Status | Opmerking |
|---|---|---|---|
| 0x01 | `ECG` | ✅ | 250 Hz, ook als een elektrode los zit (dan staat `VITALS.flags` bit2 aan) |
| 0x02 | `PPG` | ✅ | 100 Hz rood + IR |
| 0x03 | `IMU` | ✅ | 50 Hz, ±4 g / ±500 °/s |
| 0x04 | `GSR` | ✅ | 10 Hz in nS |
| 0x05 | `VITALS` | ✅ | alle velden, `reserved` = 0 |
| 0x06 | `RR` | ✅ | alleen als er slagen zijn (elektrodes vast) |
| 0x07 | `AIR` | ✅ | elke 5 s |
| 0x08 | `CABIN` | ⚠️ deels | `pressure_pa` en `pressure_trend` werken. `lux_c10`: de BH1750 is geschrapt, dus zonder demomodus altijd `0xFFFFFFFF`, in de demomodus een **gesimuleerde** waarde (SIM bit5). |
| 0x09 | `STATUS` | ⚠️ deels | `sensors_ok`, `alarms` en `fw_version` werken. Accu: het prototype draait op usb, dus zonder demomodus `vbat_mv` = `0xFFFF` en `battery_pct` = `0xFF`, in de demomodus **gesimuleerd** (SIM `extra` bit0). `sensors_ok` bit5 (BH1750) is altijd 0. |
| 0x20 | `EVENT` | ✅ | codes 0–12, behalve code 9 (accu bijna leeg): die komt op het prototype nooit |
| 0x0A | `SIM` | ✅ | uitbreiding: welke waarden gesimuleerd zijn, zie hieronder |
| – | `history` | ✅ | uitbreiding, zie hieronder |

`control` 0x01–0x04 en de uitbreidingen 0x10 en 0x11 werken allemaal. Getest via usb (`alert`, `ack`, `streams`) en op de pols; met een echt dashboard is de hele keten nog niet getest, want dat bestaat nog niet.

### Verbinden en ontgrendelen

- Naam in de advertentie: `AstroNav-Wrist`. De service-UUID zit in de advertentie, de naam in de scan response (samen passen ze niet in 31 bytes).
- **Geen koppelen (pairing) op bluetooth-niveau.** Dat werkte niet: Windows en `btleplug` konden de codevraag niet afhandelen, en een telefoon viel na het koppelen weer weg. Iedereen kan dus gewoon verbinden, zonder dat het besturingssysteem iets vraagt.
- **De app ontgrendelt het horloge zelf**, als eerste na het verbinden, met **`control` 0x13** + `u32` code (little-endian). De code is 6 cijfers, komt uit het MAC-adres (per horloge anders, maar altijd hetzelfde) en staat op het OLED zolang er niemand ontgrendeld heeft (en in de USB-monitor bij het opstarten).
- **Tot het ontgrendelen stuurt het horloge niks** op `data`, `events` en `history`, en negeert het alle andere control-commando's. Na de goede code komt er **meteen een `STATUS`- en `SIM`-pakket**: daaraan ziet de app dat het gelukt is. Komt dat niet binnen een seconde, dan was de code fout.
- Na **5 verkeerde codes** verbreekt het horloge de verbinding. Bij elk verbreken gaat hij weer op slot.
- Volgorde voor de app: verbinden → MTU 247 vragen → notify/indicate aanzetten op `data`, `events` (en `history`) → `control` 0x13 met de code → `control` 0x11 met de klok.
- Het horloge heeft **geen seriële (UART) service**. Een "Bluetooth serial"-app op de telefoon werkt daarom niet. Testen kan met nRF Connect: verbinden, notify aanzetten op `data`, en op `control` de bytes `13` + de code als u32 little-endian schrijven (bijvoorbeeld code 873320 = `0x000D5368`, dus `13 68 53 0D 00`).

### Timing en tijdstempels

- ECG, PPG, IMU en GSR worden in een eigen task gesampled op een vaste tik van 4 ms (FreeRTOS, `src/sampler.cpp`). `t_ms` komt voor alle pakketten van dezelfde klok (`millis()`, de hardwaretimer van de ESP32).
- **PPG:** de MAX30102 meet op zijn eigen 100 Hz-klok. De firmware leest de FIFO elke 40 ms en rekent de tijd terug vanaf het moment van uitlezen (10 ms per sample).
- **RR:** `t_ms` is het tijdstip van de R-piek waar het eerste interval in het pakket **begint**. Maximaal 5 intervallen per pakket, één pakket per seconde, alleen als er slagen waren. De R-pieken komen uit een uitgeklede Pan-Tompkins (`src/rpeaks.cpp`), intervallen buiten 300–2000 ms worden weggegooid.
- **AIR** gaat bij elke nieuwe meting van de SCD41, dus elke 5 s (de frequentie van NASA Tabel 6.1-1 voor CO2). VITALS en CABIN gaan elke seconde, STATUS elke 10 s en meteen als de alarmen veranderen.
- `control` 0x02 zet alleen ECG, PPG, IMU en GSR uit. RR, VITALS en de rest blijven komen.

### Ongeldige waarden

Zoals in de algemene afspraken: `0` voor hartslag en SpO2, anders de hoogste waarde van het type. Daarnaast:

| Veld | Ongeldig | Wanneer |
|---|---|---|
| `VITALS.perfusion_c10` | `0xFF` | geen geldige PPG |
| `VITALS.skin_temp_c100` | `0x7FFF` | MAX30102 niet gevonden |
| `VITALS.activity`, `tremor_hz_c10`, `tremor_amp` | `0xFF` | geen IMU |
| alle velden van `AIR`, ook `nasa_status` | max van het type | geen verse SCD41-meting (> 30 s oud) |
| `CABIN.pressure_trend` | `0x7FFF` | geen (verse) BMP180-meting |
| `STATUS.vbat_mv`, `battery_pct` | `0xFFFF`, `0xFF` | geen accu (alleen USB) |

`ppco2_c100` wordt uitgerekend met de cabinedruk van de BMP180. Zonder BMP180 neemt de firmware 1 atm (101 325 Pa, ISS). De BMP180 geeft de druk ook één keer per minuut door aan de SCD41, die zijn ppm daarmee compenseert. Een BMP180-meting met een onmogelijke temperatuur (buiten −20 tot 60 °C) telt niet, zodat het lekalarm niet vals afgaat.

### Berekend op de pols

| Veld | Hoe |
|---|---|
| `hr_ppg`, `spo2` | Maxim-algoritme (SparkFun-bibliotheek) op 25 Hz, venster van 4 s, elke seconde. Onder de IR-drempel (geen huid) niks. |
| `hr_ecg` | gemiddelde van de laatste 8 RR-intervallen, 0 na 3 s zonder slag |
| `perfusion_c10` | (max − min) / gemiddelde van het IR-signaal over 4 s |
| `skin_temp_c100` | chiptemperatuur van de MAX30102, elke 10 s. Is een trend, geen lichaamstemperatuur. |
| `activity` | rms van de versnelling boven 0,5 Hz: < 25 mg = 0 (rust), < 150 mg = 1 (licht), anders 2. Eigen keuze. |
| `tremor_amp` | rms in de 4–12 Hz band in mg (1 per mg, max 255) |
| `tremor_hz_c10` | uit de nuldoorgangen op de as met de meeste tremor, 0 onder 3 mg |
| `flags` bit1 (artefact) | activiteit > 60 mg: de PPG is dan niet te vertrouwen |
| `flags` bit3 (schok) | meer dan 3 g in de laatste seconde |

### Alarmen en buzzer

De codes, grenzen en ernst staan in [alarmen.md](alarmen.md). Elk alarm gaat als `EVENT` met `source` 0 over `events` (indicate): bij het begin, bij het einde en als het erger wordt (SpO2 van waarschuwing naar alarm).

- **Buzzer:** de patronen per ernst van hierboven. Info-gebeurtenissen van de pols zelf (elektrode los, geen huidcontact) piepen niet, anders piept hij elke keer als je hem afdoet. Een info-melding van het dashboard geeft wel één piep.
- **Dubbeltik** bevestigt alles wat op dat moment actief is: buzzer stil, de melding van het dashboard weg, en `EVENT` code 11 met `source` 2 en als `value` de code van de melding (of van het ergste alarm). Komt een alarm later opnieuw op, dan piept hij weer.
- **`control` 0x03** zet de buzzer stil voor die ene code, en haalt een dashboardmelding met die code weg.
- **`control` 0x04:** de tekst komt op het scherm (in het Engels, afgebroken voor het staande scherm), het patroon hoort bij de ernst, en de pols stuurt `EVENT` met dezelfde code, `state` 1 en `source` 1 terug. Bij `message_id` 1 start ook de ademhalingsoefening van 2 minuten.

### Uitbreidingen op v1

Deze dingen zitten niet in de afspraak hierboven. Ze breken niks: een dashboard dat ze niet kent, merkt er niks van.

**Eventcode 12: cabineklimaat.** Cabinetemperatuur of luchtvochtigheid een minuut buiten het werkgebied van NASA-STD-3001 [V2 6012] (18–27 °C, 25–75 % RV). Ernst: waarschuwing, `value` = temperatuur ×100. [V2 6022] vraagt om te waarschuwen bij temperatuur en vochtigheid, daarom deze code. In `STATUS.alarms` is het bit 12.

**`control` 0x11: klok zetten.** `u32` unix-tijd. Alleen nodig voor de tijdstempels in de geschiedenis. Het dashboard stuurt dit direct na het verbinden.

**Demomodus en pakket 0x0A `SIM`.** Voor de demo kan het horloge nepwaarden invullen voor wat op het prototype niet gemeten wordt: licht (de BH1750 is geschrapt), de accu (het prototype draait op usb), en een sensor die er niet is of wegvalt. Het dashboard moet altijd kunnen zien wat echt is, daarom gaat met elk `STATUS`-pakket (en meteen als het verandert) een `SIM`-pakket mee:

| Veld | Type | Betekenis |
|---|---|---|
| kop | 8 bytes | type 0x0A |
| `demo` | u8 | 1 = demomodus staat aan |
| `simulated` | u8 | **zelfde bits als `STATUS.sensors_ok`**. Bit aan = de waarden van die sensor zijn **gesimuleerd**, niet gemeten. Bit5 (BH1750) staat in de demomodus altijd aan. |
| `extra` | u8 | bit0 = accu (`STATUS.vbat_mv`, `battery_pct`) is gesimuleerd |

Afspraken:
- Een **echte sensor gaat altijd voor**: zolang hij werkt, wordt hij nooit vervangen door nepwaarden.
- Een sensor die wel gevonden is maar onzin meet (bijvoorbeeld de BMP180 met een onmogelijke temperatuur) wordt **niet** aangevuld, dan blijft de waarde ongeldig.
- De **ruwe streams** (ECG, PPG, IMU, GSR) en RR zijn **altijd echt**, ook in de demomodus.
- Gesimuleerde waarden kunnen alarmen laten afgaan; het `EVENT` zegt dat niet zelf, het dashboard kan het afleiden uit `SIM`.
- Op het OLED staat een `*` achter gesimuleerde waarden (bijvoorbeeld `Bat 85%*`).

Voorstel voor het dashboard: zet "SIMULATED" (of een andere kleur) bij elke waarde waarvan de bit in `simulated` of `extra` aan staat, en toon bovenaan "Demo mode" als `demo` = 1.

**`control` 0x12: demomodus.** `u8` 0 = uit, 1 = aan. Standaard aan (`DEMO_MODE` in `include/pins.h`). Uit geeft een eerlijke meting zonder vulling: wat ontbreekt, is dan gewoon ongeldig.

**Geschiedenis (characteristic `history`, `d7a1e5c2-3b4f-4c8a-9e61-0b7f2a9c4d13`, notify).** NASA-STD-3001 [V2 6001]: *"The system shall provide environmental and suit monitoring data in formats compatible with performing temporal trend analyses."* Het horloge slaat elke minuut een samenvatting op in de flash, ook zonder laptop. Er passen **7 dagen** in (10 080 minuten); daarna worden de oudste overschreven. Elk record heeft een `seq` dat altijd oploopt, ook na een herstart.

Ophalen met **`control` 0x10** + `u32 vanaf_seq`:

1. `vanaf_seq` = 0: het horloge stuurt alleen het bereik, `[0x00][u32 oudste][u32 nieuwste]`.
2. `vanaf_seq` > 0: alle records vanaf daar, als `[aantal u8][record × aantal]`. Zoveel als de MTU toelaat (max 4), elke 20 ms een pakket. Aan het eind komt het bereik nog een keer, dan weet het dashboard dat het klaar is.
3. Het dashboard onthoudt de hoogste `seq` en vraagt de volgende keer vanaf `seq + 1`.

Record, versie 2, 58 bytes, little-endian (`HistoryRecord` in `src/history.h`):

| Offset | Type | Veld | Eenheid |
|---|---|---|---|
| 0 | u8 | version | 2 |
| 1 | u32 | seq | |
| 5 | u32 | unix_time | begin van de minuut, 0 = klok niet gezet |
| 9 | u8 | flags | welke sensoren deze minuut gemeten hebben, zie hieronder |
| 10 | u8 | status | bit0 gesimuleerd, bit1 ademhalingsoefening, bit3 buzzer stil |
| 11 | u16 | alerts | alle alarmen die deze minuut aan waren (bit n = code n) |
| 13 | u8 | hr_avg | bpm |
| 14 | u8 | hr_min | bpm |
| 15 | u8 | hr_max | bpm |
| 16 | u8 | spo2_avg | % |
| 17 | u8 | spo2_min | % |
| 18 | i16 | skin_temp | 0.01 °C, chiptemperatuur MAX30102 |
| 20 | u16[12] | co2 | ppm, per 5 seconden (NASA Tabel 6.1-1) |
| 44 | u16 | ppco2_avg | 0.01 mmHg, 1-uursgemiddelde aan het eind van de minuut ([V2 6004]) |
| 46 | i16 | cabin_temp | 0.01 °C |
| 48 | u16 | humidity | 0.01 % |
| 50 | u16 | pressure | 0.1 hPa, cabinedruk |
| 52 | u16 | gsr | 0.01 µS |
| 54 | u8 | activity | % van de minuut niet in rust |
| 55 | u8 | tremor | mg, hoogste waarde |
| 56 | u16 | battery_mv | mV, laatste meting |

`flags`: bit0 hartslag/SpO2, bit1 huidtemperatuur, bit2 SCD41, bit3 cabinedruk, bit4 IMU, bit5 GSR, bit6 ECG-elektrodes vast, bit7 klok gezet. Een waarde telt alleen als de bijbehorende bit aan staat.

### Testen zonder dashboard (USB, 115200)

| Commando | Wat |
|---|---|
| `info` | naam, code, verbonden of niet |
| `sensors` | welke sensoren gevonden zijn |
| `show` | alle meetwaarden en actieve alarmen |
| `alert <code> <ernst> <tekst>` | doet hetzelfde als `control` 0x04 |
| `ack` | doet hetzelfde als een dubbeltik |
| `streams on/off` | `control` 0x02 |
| `sim on/off`, `sim <veld> <waarde>` | demomodus aan/uit (= `control` 0x12), of een vaste waarde (`co2`, `spo2`, `hr`, `temp`, `cabin`, `pressure`) |
| `breathe [min]`, `breathe off` | ademhalingsoefening |
| `history`, `history <n>`, `history wipe` | bereik, laatste n minuten als tekst, alles wissen |
| `time <unix>` | klok zetten |
| `reg <adres> <reg> [n]` | i2c-register uitlezen (hex), om een sensor na te kijken |
| `bmp`, `oled`, `buzz`, `reboot` | BMP180 nakijken, scherm opnieuw zoeken, piep, herstart |
