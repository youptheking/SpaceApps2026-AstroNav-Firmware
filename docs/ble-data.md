# BLE-data – wat elke sensor kan leveren

Brainstormdocument voor het formaat van de BLE-pakketten tussen het polsstation en het dashboard.

Legenda:
- **Ruw:** komt direct uit de sensor.
- **Afgeleid:** berekend uit ruwe data, op het polsstation of in het dashboard.
- **Waar:** waar je de berekening het best doet. *Pols* = nodig voor de lokale alarmen, die zonder laptop moeten werken. *Dashboard* = zwaardere analyse.

## MAX30102 – optisch (PPG)

| Data | Soort | Frequentie | Waar | Waarvoor in het dashboard |
|---|---|---|---|---|
| Rood licht (18-bit) | ruw | 100 Hz | – | PPG-golf tonen |
| Infrarood licht (18-bit) | ruw | 100 Hz | – | PPG-golf tonen |
| Hartslag (bpm) | afgeleid | 1 Hz | pols | Vitale waarde, alarm |
| SpO2 (%) | afgeleid | 1 Hz | pols | Vitale waarde, alarm bij een daling |
| Perfusie-index (AC/DC-verhouding) | afgeleid | 1 Hz | dashboard | Kwaliteit van de doorbloeding en van het signaal |
| Ademfrequentie | afgeleid | elke ~30 s | dashboard | Afleiden uit de modulatie van de PPG-golf. Experimenteel. |
| Temperatuur (chip) | ruw | 0,1 Hz | – | Trend van de huidtemperatuur |
| Huidcontact (hoogte van het IR-niveau) | afgeleid | 1 Hz | pols | "Sensor niet op de huid" |

## AD8232 – elektrisch (ECG)

| Data | Soort | Frequentie | Waar | Waarvoor in het dashboard |
|---|---|---|---|---|
| ECG-signaal (12-bit ADC) | ruw | 250 Hz | – | Hartfilm-golf tonen |
| LO+ / LO− | ruw | bij verandering | pols | "Elektrode los" |
| R-pieken (tijdstippen) | afgeleid | per slag | pols | Basis voor hartslag en HRV |
| Hartslag uit ECG (bpm) | afgeleid | 1 Hz | pols | Tweede onafhankelijke bron naast de PPG |
| RR-intervallen (ms) | afgeleid | per slag | pols | Tachogram, basis voor HRV |
| HRV: RMSSD, SDNN | afgeleid | elke ~60 s | dashboard | **Stress en vermoeidheid** (isolatie) |
| Onregelmatig ritme | afgeleid | bij een gebeurtenis | dashboard | Melding bij een sterk afwijkend RR-interval |

## Combinatie ECG + PPG

| Data | Soort | Waar | Waarvoor |
|---|---|---|---|
| **Pulse Transit Time (PTT):** tijd van de R-piek in de ECG tot de aankomst van de polsgolf in de PPG | afgeleid | dashboard | **Trend van de bloeddruk** (relatief, niet absoluut). Relevant voor de belasting van hart en vaten in gewichtloosheid. Werkt alleen als ECG en PPG nauwkeurig op dezelfde klok worden getimestampt. |
| Overeenkomst tussen de hartslag uit ECG en PPG | afgeleid | dashboard | Betrouwbaarheid van het signaal. Wijken ze af, dan is één van de twee verstoord. |

## MPU-6050 – beweging

| Data | Soort | Frequentie | Waar | Waarvoor in het dashboard |
|---|---|---|---|---|
| Versnelling x, y, z | ruw | 50 Hz | – | Bewegingsgrafiek |
| Rotatie x, y, z | ruw | 50 Hz | – | Bewegingsgrafiek |
| Activiteitsniveau | afgeleid | 1 Hz | pols | Rust, licht of intensief (training) |
| Tremor (4–12 Hz in het frequentiespectrum) | afgeleid | elke ~10 s | dashboard | Spiertremoren, vermoeidheid |
| Rust/slaap | afgeleid | per minuut | dashboard | Slaapduur en -kwaliteit |
| Schok of botsing | afgeleid | bij een gebeurtenis | pols | Melding |
| Bewegingsartefact-vlag | afgeleid | 1 Hz | pols | Markeert ECG/PPG-data als onbetrouwbaar tijdens beweging |
| Pols optillen | afgeleid | bij een gebeurtenis | pols | OLED aan |

**Let op in gewichtloosheid:** een accelerometer meet op aarde de zwaartekracht, en daaruit leid je de stand van de pols af. In de ruimte valt die weg. Stappen tellen en houding bepalen werken dan niet, beweging meten wel. Benoem dit eerlijk in de pitch, dat versterkt "Validity".

## SCD41 – cabinelucht

| Data | Soort | Frequentie | Waar | Waarvoor in het dashboard |
|---|---|---|---|---|
| CO2 (ppm) | ruw | elke 5–60 s | – | Grafiek |
| Temperatuur, luchtvochtigheid | ruw | elke 5–60 s | – | Indicatie (de temperatuur wordt beïnvloed door de pols) |
| ppCO2 (mmHg) | afgeleid | idem | pols | Met de cabinedruk van de BMP180 |
| 1-uursgemiddelde ppCO2 | afgeleid | idem | pols | **NASA-STD-3001-alarm** (> 3 mmHg) |
| Status ten opzichte van de NASA-grens | afgeleid | idem | pols | OK / waarschuwing / alarm |

## BMP180 – cabinedruk

| Data | Soort | Frequentie | Waar | Waarvoor in het dashboard |
|---|---|---|---|---|
| Luchtdruk (Pa) | ruw | 1 Hz | – | Grafiek van de cabinedruk |
| Temperatuur | ruw | 1 Hz | – | Reserve |
| Druktrend (Pa/min) | afgeleid | 1 Hz | pols | **Lekdetectie:** melding bij een aanhoudende daling |

## BH1750 – licht

| Data | Soort | Frequentie | Waar | Waarvoor in het dashboard |
|---|---|---|---|---|
| Lichtsterkte (lux) | ruw | 1 Hz | – | Grafiek |
| Lichtdosis (lux·uur) | afgeleid | per uur | dashboard | Lichtblootstelling over de dag |
| Licht/donker-ritme | afgeleid | per dag | dashboard | Circadiaans ritme, samen met de slaapdata |

## GSR – huidgeleiding

| Data | Soort | Frequentie | Waar | Waarvoor in het dashboard |
|---|---|---|---|---|
| Geleiding (µS, berekend uit de spanningsdeler) | ruw | 10 Hz | – | Grafiek |
| Basisniveau (tonisch) | afgeleid | elke ~10 s | dashboard | Algemene stress-staat |
| Stresspieken (fasisch, aantal per minuut) | afgeleid | per minuut | dashboard | Acute stressmomenten |

## Systeem

| Data | Frequentie | Waarvoor |
|---|---|---|
| Accuspanning en percentage | 0,1 Hz | Batterij-indicator, geschatte resterende tijd |
| Volgnummer van het pakket | elk pakket | Verloren pakketten detecteren |
| Tijdstempel (ms sinds de start) | elk pakket | Data op één tijdlijn, nodig voor PTT en HRV |
| Status van elke sensor (OK / fout / niet aanwezig) | 1 Hz | Toon in het dashboard welke sensor werkt |
| Actieve alarmen | bij verandering | Gelijk met het polsstation |
| Firmwareversie | bij het verbinden | Debuggen |
| RSSI (signaalsterkte) | – | Meet het dashboard zelf |

## Datavolume

| Stroom | Berekening | Bytes/s |
|---|---|---|
| ECG | 250 Hz × 2 bytes | 500 |
| PPG (rood + IR) | 100 Hz × 2 × 3 bytes | 600 |
| IMU (6 assen) | 50 Hz × 6 × 2 bytes | 600 |
| GSR | 10 Hz × 2 bytes | 20 |
| Langzame data (vitaal, omgeving, systeem) | samen | < 100 |
| Kop per pakket (volgnummer, tijd, type) | ~10 pakketten/s × 8 bytes | 80 |
| **Totaal** | | **~1,9 kB/s** |

Dat past ruim binnen BLE, mits de **MTU wordt vergroot** (bijv. 247 bytes) en pakketten worden **gebundeld** (~10 per seconde in plaats van per sample). Met de standaard-MTU van 23 bytes wordt het krap.

**Kosten:** continu ruwe data versturen houdt de radio vaker actief. Reken op een paar mA extra ten opzichte van alleen samenvattingen (zie [stroombudget.md](stroombudget.md)).

## Voorstel voor de structuur (nog te beslissen)

Aparte BLE-characteristics per soort data:

| Characteristic | Inhoud | Frequentie |
|---|---|---|
| `stream` | ECG, PPG, IMU, GSR in een batch van ~100 ms | ~10/s |
| `vitals` | Hartslag (PPG en ECG), SpO2, RR-intervallen, activiteit, artefact-vlag | 1/s |
| `environment` | CO2, ppCO2, het 1-uursgemiddelde, temperatuur, luchtvochtigheid, druk, lux | per meting |
| `status` | Accu, sensorstatus, actieve alarmen | 0,1/s en bij een verandering |
| `events` | Alarmen, elektrode los, schok | bij een gebeurtenis |

Elk pakket begint met: **type (1 byte), volgnummer (2 bytes), tijdstempel in ms (4 bytes).**
