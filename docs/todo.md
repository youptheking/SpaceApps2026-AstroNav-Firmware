# Todo – NASA Space Apps 2026

**Za 14 nov 09:00 → zo 15 nov 23:59** · inleveren vóór **22:00**

Volgorde: **frezen → solderen → testen → firmware (AI) → dashboard → zondag afmaken en inleveren**

Legenda: [x] klaar · [ ] open · ⚠️ belangrijk · 🎯 MVP (moet werken) · ➕ bonus (alleen als de MVP staat)

---

## ✅ Klaar

- [x] Onderdelen besteld · ontwerpkeuzes, stroombudget en pinout gedocumenteerd
- [x] NASA-bron: NASA-STD-3001 (V2 6004 CO2-grens, V2 6022 lokaal + op afstand waarschuwen)
- [x] Vermelding van AI-gebruik (SI) in beide READMEs
- [x] Schema + PCB ontworpen (R3 op BAT_SW+)
- [x] BLE-protocol v1: `docs/ble-protocol.md` · `include/astronav_protocol.h`
- [x] Sensorcheck-sketch klaar: `tools/sensor_check/sensor_check.ino`

---

## 1. PCB – nu

- [ ] PCB frezen en boren *(bijna klaar)*
- [ ] ⚠️ Doormeten op kortsluiting **vóór** solderen: 3V3–GND, BAT–GND
- [ ] Footprints vergelijken met de echte modules (vooral OLED en de SCD41-connector)
- [ ] Solderen: headers, R1 R3 R4 R7, C1–C4, schakelaar (R5/R6 **niet**)

## 2. Hardware testen

- [ ] Sensorcheck uploaden en draaien
- [ ] SDA in rust ~3,3 V (niet ~1,8 V)
- [ ] Alle 5 I2C-adressen gevonden (0x3C 0x57 0x62 0x68 0x77)
- [ ] I2C-pull-up totaal 2–5 kΩ, zo nodig pull-ups van modules halen
- [ ] Buzzer piept, accuspanning klopt met de multimeter
- [ ] AD8232: pieken in de Serial Plotter, LO+/LO− reageren (plaatsing: `docs/ecg-elektrodes.md`)
- [ ] GSR: 15-minutentest. Reageert hij? Houden. Alleen ruis? Laten vallen.

## 3. Firmware (met AI)

- [ ] 🎯 MAX30102: hartslag + SpO2 → `PPG` + `VITALS`
- [ ] 🎯 SCD41: CO2 → ppCO2, 1-uursgemiddelde, NASA-status → `AIR`
- [ ] 🎯 OLED + buzzer: lokale alarmen, werken **zonder laptop**
- [ ] 🎯 BLE: service + `data` / `events` / `control`, MTU 247
- [ ] 🎯 `control` 0x04: melding van het dashboard → OLED + buzzerpatroon
- [ ] 🎯 `STATUS`: accu + sensorstatus
- [ ] ⚠️ ECG- en PPG-samples op **dezelfde hardwaretimer** (nodig voor HRV en later PTT)
- [ ] ➕ AD8232: `ECG` + R-pieken → `RR`
- [ ] ➕ MPU-6050: `IMU`, activiteit, schok, artefact-vlag, dubbeltik om te bevestigen
- [ ] ➕ BMP180: cabinedruk + lekalarm → `CABIN`
- [x] ~~BH1750~~ geschrapt (footprint klopte niet, te veel tijd)
- [ ] ➕ GSR → `GSR`
- [ ] Stroomverbruik meten via de schakelaarpinnen

## 4. Dashboard (pc-software) – hoofdfocus

- [ ] 🎯 Tauri-project draait
- [ ] 🎯 BLE-verbinding (btleplug), MTU 247, pakketten uitlezen volgens het protocol
- [ ] 🎯 Live grafieken: hartslag, SpO2, CO2
- [ ] 🎯 CO2-grafiek met de **NASA-STD-3001-lijnen** (3 en 2,5 mmHg) + 1-uursgemiddelde
- [ ] 🎯 Beslispaneel: concrete adviezen, met de bron bij elk alarm
- [ ] 🎯 Melding terug naar de pols (`control` 0x04)
- [ ] ⚠️🎯 **Een uur echte data opnemen** → demomodus (werkt zonder hardware)
- [ ] ➕ ECG-golf + HRV (RMSSD) + stressmelding naar de pols
- [ ] ➕ Trends over tijd: slaap, stress, temperatuur, activiteit
- [ ] ➕ Beweging, cabinedruk, GSR
- [ ] ➕ PTT (bloeddruktrend), alleen als echt alles af is
- [ ] AI-vermelding bovenaan elk bestand met AI-code

## 5. Behuizing

- [ ] Ontwerp: USB-C, antenne, geluidsgat, SCD41-rooster, venster voor OLED en BH1750, lussen voor het klittenband
- [ ] Printen (eventueel 's nachts)
- [ ] Alles erin, om de pols, getest

---

## 6. Zondag – inleveren

- [ ] ⚠️ **18:00: geen nieuwe functies meer**
- [ ] SpO2-drempels (94 % / 90 %) nalezen in de WHO Pulse Oximetry Training Manual
- [ ] Video **max 30 s**, **Engelse ondertitels**, geen personen onder de 18, op YouTube
  - Noem: **zwaartekrachtpunt** (`ontwerpkeuzes.md` punt 8), NASA-grens, lokaal + op afstand waarschuwen
- [ ] Projectpagina in het Engels:
  - [ ] Project Name
  - [ ] High-Level Summary
  - [ ] Link to Project Demo
  - [ ] Link to Final Project (GitHub)
  - [ ] Detailed Project Description
  - [ ] Use of SI
  - [ ] NASA Data (tekst in `docs/nasa-std-3001.md`, nieuwste revisie checken)
  - [ ] References: libraries, datasheets, NASA- en WHO-documenten
- [ ] ⚠️ Bron van `docs/img/ecg-elektrodes.png` invullen (of de afbeelding vervangen door een eigen tekening)
- [ ] Engelse samenvatting in de READMEs
- [ ] ⚠️ **Beide repo's openbaar**, links getest in een privévenster
- [ ] Vinkjes "Terms and Conditions" + "Confirm All Work is Original"
- [ ] ⚠️ **Submit for Judging vóór 22:00**
- [ ] Certificaat downloaden
