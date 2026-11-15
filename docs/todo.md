# Todo – NASA Space Apps 2026

**Za 14 nov 09:00 → zo 15 nov 23:59** · inleveren vóór **22:00**

Legenda: [x] klaar · [ ] open · ⚠️ belangrijk · 🎯 MVP (moet werken) · ➕ bonus · ~~doorgestreept~~ geschrapt

---

## Vandaag nog, op volgorde

1. [x] BLE-test op de laptop: verbinden en ontgrendelen werkt
2. [ ] ⚠️ Sensordata kloppend maken in het dashboard *(bezig)*, daarna melding terug naar de pols en de SIM-labels nalopen
3. [ ] Behuizing printen en alles erin
4. [ ] Video opnemen (kan met de demomodus als de hardware tegenzit)
5. [ ] **18:00: geen nieuwe functies meer**
6. [ ] Documentatie op orde (zie punt 6), projectpagina invullen, repo's openbaar
7. [ ] ⚠️ **Submit for Judging vóór 22:00**

---

## ✅ Klaar

- [x] Onderdelen besteld · ontwerpkeuzes, stroombudget en pinout gedocumenteerd
- [x] NASA-bron: NASA-STD-3001 (V2 6004 CO2-grens, V2 6022 lokaal + op afstand waarschuwen)
- [x] Schema + PCB ontworpen (R3 op BAT_SW+)
- [x] BLE-protocol v1: `docs/ble-protocol.md` · `include/astronav_protocol.h`
- [x] Sensorcheck-sketch: `tools/sensor_check/sensor_check.ino`

## 1. PCB

- [x] PCB frezen en boren
- [x] Doormeten op kortsluiting
- [x] Solderen, inclusief R5/R6 (4,3 kΩ pull-ups bleken toch nodig)
- [x] MAX30102 met draadjes aangesloten (footprint klopte niet)
- [x] 0.96" OLED in plaats van de 0.91", 90° gedraaid
- [ ] ~~Accuconnector en aan/uit-schakelaar~~ afgebroken, het horloge draait op usb

## 2. Hardware testen

- [x] Alle I2C-adressen gevonden, 0 fouten op de bus
- [x] OLED, buzzer, SCD41, MPU-6050 (reserve-module), MAX30102 (reserve-module)
- [ ] BMP180: leest af en toe onzin (~218 kPa), wordt weggefilterd. Soldeerpunten nalopen.
- [ ] ⚠️ MAX30102: hartslag en SpO2 met een vinger erop, liefst naast een echte saturatiemeter
- [ ] AD8232: ECG met elektrodes op de borst (plaatsing: `docs/ecg-elektrodes.md`)
- [ ] GSR: reageert hij op de huid?
- [ ] ~~Accuspanning~~ geen accu

## 3. Firmware

- [x] 🎯 Opstarten met 2 s wachten, ontbrekende sensoren elke 10 s opnieuw zoeken
- [x] 🎯 MAX30102: hartslag + SpO2 → `PPG` + `VITALS`
- [x] 🎯 SCD41: CO2 → ppCO2, 1-uursgemiddelde, NASA-status → `AIR`
- [x] 🎯 OLED + buzzer: lokale alarmen, werken zonder laptop
- [x] 🎯 BLE: `data` / `events` / `control` / `history`, MTU 247
- [x] 🎯 BLE zonder koppelen: de app ontgrendelt met de code van het scherm (`control` 0x13)
- [x] 🎯 `control` 0x04: melding van het dashboard → OLED + buzzerpatroon
- [x] 🎯 `STATUS` met sensorstatus, sensorscherm bij opstarten en bij een wijziging
- [x] ECG, PPG, IMU en GSR op dezelfde 4 ms-klok
- [x] ➕ AD8232: `ECG` + R-pieken → `RR`
- [x] ➕ MPU-6050: `IMU`, activiteit, schok, tremor, dubbeltik om te bevestigen
- [x] ➕ BMP180: cabinedruk + lekalarm → `CABIN`
- [x] ➕ GSR → `GSR`
- [x] ➕ 7 dagen geschiedenis in de flash
- [x] Demomodus met SIM-pakket: nepwaarden zijn altijd gemarkeerd
- [x] UI in het Engels
- [ ] ~~BH1750~~ geschrapt (footprint klopte niet)
- [ ] ~~Stroomverbruik meten~~ geen accu meer
- [x] ⚠️ Getest met de laptop via BLE

## 4. Dashboard

- [x] 🎯 Tauri-project draait
- [x] 🎯 BLE (btleplug), pakketten uitlezen volgens het protocol, ontgrendelen met de code
- [x] 🎯 Live grafieken en ISS-console-layout
- [x] 🎯 CO2 met de NASA-STD-3001-grenzen
- [x] 🎯 Beslispaneel met adviezen
- [x] 🎯 Melding terug naar de pols (`control` 0x04)
- [x] 🎯 Demomodus in het dashboard (gesimuleerd horloge, werkt zonder hardware)
- [x] ➕ HRV-trend, lichaamskaart met tabs Systems en ECG leads (springt naar ECG leads als een elektrode los zit)
- [x] YoupSpace-logo linksboven
- [ ] Space Apps-logo: bestand toevoegen, de plek staat klaar
- [x] ⚠️ Verbinding met het echte horloge werkt
- [ ] ⚠️ Sensordata klopt in het dashboard *(bezig)*
- [ ] Geschiedenis ophalen (`history`) wordt nog niet gebruikt
- [ ] ~~PTT~~ alleen als echt alles af is
- [ ] ~~AI-vermelding bovenaan elk bestand~~ besloten: niet in de bestanden, wel in de README en op de projectpagina

## 5. Behuizing

- [ ] Ontwerp: USB-C (altijd aangesloten), antenne, geluidsgat, SCD41-rooster, venster voor de OLED, lussen voor het klittenband
- [ ] Printen
- [ ] Alles erin, om de pols: dubbeltik, schermstand en buzzer nog te horen?

---

## 6. Inleveren

- [ ] ⚠️ **18:00: geen nieuwe functies meer**
- [ ] ⚠️ Documenten samenvoegen: `nasa-std-3001.md`, `ontwerpkeuzes.md`, `ecg-elektrodes.md` (+ afbeelding), `pinout.md`, `pcb.md`, `stroombudget.md`, deze todo en de sensorcheck staan op de branch `ccr-5b998f8a-3pwruq`, niet op `setup/platformio`
- [ ] ⚠️ Sectie "Gebruik van AI (SI)" terug in de README van de firmware (staat alleen op die andere branch), en controleren in die van het dashboard. Let op: daar staat nog "een opmerking in het betreffende bestand", dat klopt niet meer.
- [ ] ⚠️ PR's mergen naar `main`, zodat de GitHub-link de laatste versie laat zien
- [ ] SpO2-drempels (94 % / 90 %) nalezen in de WHO Pulse Oximetry Training Manual
- [ ] Video **max 30 s**, **Engelse ondertitels**, geen personen onder de 18, op YouTube
  - Noem: zwaartekrachtpunt (`ontwerpkeuzes.md` punt 8), NASA-grens, lokaal + op afstand waarschuwen
- [ ] Projectpagina in het Engels:
  - [ ] Project Name
  - [ ] High-Level Summary
  - [ ] Link to Project Demo
  - [ ] Link to Final Project (GitHub)
  - [ ] Detailed Project Description
  - [ ] Use of SI
  - [ ] NASA Data (tekst in `docs/nasa-std-3001.md`, nieuwste revisie checken)
  - [ ] References: libraries, datasheets, NASA- en WHO-documenten
- [ ] Bron van `docs/img/ecg-elektrodes.png` invullen (of een eigen tekening)
- [ ] Engelse samenvatting in de READMEs
- [ ] ⚠️ **Beide repo's openbaar**, links getest in een privévenster
- [ ] Vinkjes "Terms and Conditions" + "Confirm All Work is Original"
- [ ] ⚠️ **Submit for Judging vóór 22:00**
- [ ] Certificaat downloaden
