# NASA-STD-3001 – gebruikte grenswaarden

**NASA-bron voor het project:** *NASA Space Flight Human-System Standard (NASA-STD-3001), Volume 2: Human Factors, Habitability, and Environmental Health.*

De alarmen van het polsstation en de referentielijnen in het dashboard komen uit deze norm, niet uit eigen inschatting.

> **Controleer de getallen in de actuele revisie voordat je ze als definitief gebruikt.** De waarden hieronder komen uit de NASA-documenten in de bronnenlijst. Er bestaan meerdere revisies (A t/m F). De CO2-grens is in alle gevonden bronnen hetzelfde. Bij temperatuur en luchtvochtigheid verschillen de bronnen, dat staat erbij.

## 1. CO2 – de belangrijkste

| Grens | Waarde | Bron |
|---|---|---|
| **Gemiddelde over 1 uur** | **≤ 3 mmHg ppCO2** (bij 760 mmHg cabinedruk) | Requirement **V2 6004**, sectie 6.2.1.3 |
| Doel voor langere termijn (gemiddelde over 7 dagen) | < 2,5 mmHg | NASA OCHMO Technical Brief CO2 |
| Waarom | 3 mmHg houdt het percentage hoofdpijnklachten onder ~1,4 %, 2,5 mmHg onder ~1 % | NASA OCHMO Technical Brief CO2 |

**Omrekenen naar ppm** (de SCD41 meet in ppm): `ppm = mmHg / cabinedruk × 1.000.000`

| mmHg | ppm bij 760 mmHg |
|---|---|
| 2,5 | **≈ 3290 ppm** |
| 3,0 | **≈ 3950 ppm** |

**Bevestiging van de sensorkeuze:** NASA's grens ligt rond 3950 ppm. De SCD40 is maar gespecificeerd tot 2000 ppm en zou deze grens dus niet betrouwbaar kunnen meten. De SCD41 (tot 5000 ppm) kan dat wel.

**Let op de cabinedruk:** de norm spreekt over **partiële druk** (mmHg), de SCD41 meet een **concentratie** (ppm). Bij een andere cabinedruk dan 760 mmHg verschuift de ppm-grens, bijvoorbeeld bij een lagere druk in een maanlander. Daarom meet een **BMP180** de cabinedruk, en rekent de firmware met de gemeten druk. Zonder BMP180, of zolang die nog niet werkt, valt de firmware terug op 760 mmHg (zoals op het ISS). De BMP180 heeft een absolute nauwkeurigheid van ~±1 hPa, dus ~0,1 %. Dat is verwaarloosbaar voor de CO2-grens.

De gemeten druk gaat ook naar de SCD41 (`setAmbientPressure`), zodat die zijn CO2-meting compenseert. Let op de eenheid: afhankelijk van de libraryversie is dat hPa of Pa.

## 2. Alarmering: precies wat het ontwerp al doet

Requirement **V2 6022** (sectie 6.2.6.1): het systeem moet de bemanning **lokaal én op afstand** waarschuwen als atmosferische parameters buiten de veilige grenzen komen: **cabinedruk, luchtvochtigheid, temperatuur, ppO2 en ppCO2**. Het polsstation meet daarvan cabinedruk (BMP180, lekalarm), luchtvochtigheid en ppCO2.

| Eis | Ons ontwerp |
|---|---|
| Lokaal waarschuwen | OLED + buzzer op het polsstation, werkt ook zonder laptop |
| Op afstand waarschuwen | Melding in het dashboard via BLE |

Dit is een sterk punt voor de pitch: de architectuur volgt een eis uit de NASA-norm.

## 3. Temperatuur en luchtvochtigheid in de cabine (de SCD41 meet die ook)

| Parameter | Waarde | Bron | Zekerheid |
|---|---|---|---|
| Temperatuur | 18–27 °C (nominaal) | V2 6013, Revision A | Oudere revisie; een technical brief op basis van Rev D noemt 20–25 °C als werkbereik |
| Relatieve luchtvochtigheid | 25–75 % (gemiddeld over 24 uur) | Revision A en B | Technical brief noemt 30–60 % als werkbereik |

**Beperking:** de SCD41 zit om de pols. De **temperatuur** die hij meet wordt beïnvloed door lichaamswarmte, dus gebruik die niet voor alarmen, hooguit als indicatie. De **luchtvochtigheid** is minder gevoelig en kan wel tegen het bereik worden gezet.

## 4. Zo zit het in het project

**Firmware (polsstation):**
```cpp
// Grenswaarden volgens NASA-STD-3001 Vol. 2 (zie docs/nasa-std-3001.md)
// Cabinedruk gemeten met de BMP180, terugval op 760 mmHg
float cabinPressureMmHg = 760.0f;          // overschreven door BMP180 (hPa × 0,750062)
constexpr float CO2_LIMIT_1H_MMHG     = 3.0f;   // V2 6004, 1-uursgemiddelde
constexpr float CO2_TARGET_MMHG       = 2.5f;   // OCHMO technical brief, lange termijn

inline float ppmToMmHg(float ppm) { return ppm * cabinPressureMmHg / 1e6f; }
```
- **Alarm** (OLED "CO2 hoog" + buzzer): het **voortschrijdend 1-uursgemiddelde** komt boven 3 mmHg. Dit volgt de norm letterlijk.
- **Vroege waarschuwing** (alleen OLED): de actuele waarde komt boven 3 mmHg. Zo ziet de astronaut het aankomen voordat het uurgemiddelde de grens bereikt.

**Dashboard:**
- CO2-grafiek met een **rode lijn op 3 mmHg (≈ 3950 ppm)** en het label "NASA-STD-3001 limit", plus een gestippelde lijn op 2,5 mmHg met "NASA long-term target".
- Toon ook het **1-uursgemiddelde** naast de actuele waarde, want de norm gaat over dat gemiddelde.
- Bij een melding in het beslispaneel staat erbij waarom: *"1-hour average ppCO2 above NASA-STD-3001 limit (3 mmHg)"*.

## 5. Tekst voor het veld "NASA Data" (Engels)

> We used **NASA-STD-3001 Volume 2 (NASA Space Flight Human-System Standard)** as the basis for our alarm logic. The wearable measures cabin pressure (BMP180), converts the SCD41 CO2 reading to partial pressure and raises a local alarm (display and buzzer) when the one-hour average ppCO2 exceeds the 3 mmHg limit of requirement V2 6004. The dashboard shows this limit and the 2.5 mmHg long-term target from NASA's OCHMO CO2 technical brief as reference lines. Our architecture of local alerts on the wrist plus remote alerts on the dashboard follows the crew alerting requirement in section 6.2.6.1. The standard also drove our sensor choice: we selected the SCD41 over the cheaper SCD40 because the SCD40's specified range (2000 ppm) does not cover NASA's limit of roughly 3950 ppm.

## Bronnen

- [NASA OCHMO Technical Brief – Carbon Dioxide](https://www.nasa.gov/wp-content/uploads/2023/12/ochmo-tb-004-carbon-dioxide.pdf)
- [NASA-STD-3001 Vol. 2, Revision D](https://standards.nasa.gov/sites/default/files/standards/NASA/D/nasa-std-3001-vol-2-rev-d-signature.pdf)
- [NASA-STD-3001 Vol. 2, Revision C](https://standards.nasa.gov/sites/default/files/standards/NASA/C/2022-04-08-NASA-STD-3001-Vol-2-Rev-C-Final.pdf)
- [NASA-STD-3001 Vol. 2, Rev F – Appendix D (compliance matrix)](https://www.nasa.gov/wp-content/uploads/2025/07/nasa-std-3001-vol-2-app-d-requirements-compliance-matrix.pdf)
- [NASA OCHMO Technical Brief – Habitable Atmosphere](https://www.nasa.gov/wp-content/uploads/2023/12/ochmo-tb-003-habitable-atmosphere.pdf)
- [NASA OCHMO Technical Brief – Behavioral Health](https://www.nasa.gov/wp-content/uploads/2023/03/behavioral-health-technical-brief-ochmo-1.pdf) (waarom stress bewaken)
- [NASA OCHMO Technical Brief – ECLSS](https://www.nasa.gov/wp-content/uploads/2023/07/eclss-technical-brief-ochmo.pdf) (V2 6022, alarmering)
- [NASA-STD-3001 Vol. 2, Revision A](https://standards.nasa.gov/sites/default/files/standards/NASA/B/0/Historical/nasa-std-3001_vol_2_rev_a.pdf)
