# Todo – NASA Space Apps 2026 (za 14 nov 09:00 → zo 15 nov 23:59)

Legenda: [x] klaar · [ ] open · ⚠️ belangrijk

## Voorbereiding (klaar)

- [x] Onderdelen gekozen en besteld (TinyTronics, €94,45)
- [x] Ontwerpkeuzes, stroombudget en pinout gedocumenteerd
- [x] NASA-bron gekozen: NASA-STD-3001 (CO2-grens 3 mmHg, lokaal en op afstand waarschuwen)
- [x] Vermelding van AI-gebruik (SI) in beide READMEs
- [x] Schema getekend
- [x] PCB ontworpen

## Zaterdag – hardware

- [x] Label van R3 (accudeler) op **BAT_SW+**
- [ ] ⚠️ Pinvolgorde van elke footprint vergeleken met de echte modules (vooral OLED en de SCD41-connector)
- [ ] Gecontroleerd in Makera Studio: alleen de onderkant frezen klopt als er geen sporen op de toplaag liggen
- [ ] PCB frezen en boren
- [ ] Doorgemeten op kortsluiting (3V3–GND, BAT–GND) vóór er iets op gaat
- [ ] Headers en losse onderdelen solderen (R1, R3, R4, R7, C1–C4, schakelaar)
- [ ] I2C-scan: alle 6 adressen zichtbaar (0x23, 0x3C, 0x57, 0x62, 0x68, 0x77)
- [ ] SDA in rust ~3,3 V (niet 1,8 V door de MAX30102)
- [ ] Totale I2C-pull-up gemeten (doel 2–5 kΩ), zo nodig pull-ups van modules verwijderd
- [ ] Elke sensor los getest: MAX30102 → MPU-6050 → SCD41 → BH1750 → BMP180 → OLED
- [ ] AD8232: ECG-pieken zichtbaar in de Serial Plotter, LO+/LO− reageren
- [ ] Buzzer op D7 (stil bij het opstarten)
- [ ] Accuspanning op D8 klopt met de multimeter (×2)
- [ ] GSR: 15-minutentest. Werkt het? Houden. Alleen ruis? Laten vallen, stress via HRV.
- [ ] ⚠️ **Formaat van de BLE-pakketten vastgelegd:** voorstel staat in `docs/ble-protocol.md`, wacht op akkoord
- [ ] SpO2-drempels (94 % / 90 %) nalezen in de WHO Pulse Oximetry Training Manual
- [ ] Firmware: alle sensoren uitlezen, **elke meting op zijn eigen interval** (zie `docs/ble-data.md`), en over BLE versturen
- [ ] Firmware: ECG- en PPG-samples met een tijdstempel van dezelfde hardwaretimer (nodig voor HRV, en later eventueel PTT)
- [ ] Firmware: IMU-verwerking op de ESP32 (activiteit, schok, artefact-vlag, tremor)
- [ ] Firmware: CO2-alarm op het 1-uursgemiddelde > 3 mmHg (NASA-STD-3001), met de BMP180 of met 760 mmHg als terugval
- [ ] Firmware: OLED-meldingen en buzzer bij alarm, werkt zonder laptop
- [ ] Stroomverbruik gemeten via de schakelaarpinnen
- [ ] Behuizing ontworpen, **3D-print 's nachts laten lopen**

## Zaterdag – software

- [ ] Tauri-project draait (leeg venster)
- [ ] BLE-verbinding met het polsstation (btleplug)
- [ ] Live grafieken: hartslag, SpO2, ECG, CO2, beweging, licht, GSR
- [ ] CO2-grafiek met de NASA-STD-3001-lijnen (3 mmHg en 2,5 mmHg) en het 1-uursgemiddelde
- [ ] ⚠️ **Een uur echte sensordata opnemen** en opslaan in de repo (demomodus en reserve)
- [ ] AI-vermelding bovenaan elk bestand met AI-code

## Zondag – afmaken

- [ ] Alles in de behuizing, om de pols, getest
- [ ] Beslispaneel: concrete adviezen ("Neem rust", "CO2 hoog", "Elektrode los")
- [ ] Trends over tijd (slaap, stress, temperatuurtrend, activiteit)
- [ ] Demomodus: dashboard speelt opgenomen data af zonder hardware
- [ ] ⚠️ **18:00: geen nieuwe functies meer**

## Extra – alleen als alles af is

- [ ] **Pulse Transit Time (PTT):** bloeddruktrend uit de tijd tussen de R-piek (ECG) en de aankomst van de polsgolf (PPG). Puur software in het dashboard. **Voorwaarde, die nu al in de firmware moet:** ECG- en PPG-samples krijgen een tijdstempel van dezelfde klok (een hardwaretimer van de ESP32).

## Zondag – inleveren

- [ ] Video: **noem het zwaartekrachtpunt**. Op aarde gebruikt de IMU de zwaartekracht voor de stand van de pols, in gewichtloosheid valt die weg. Beweging meten werkt wel. Zie `docs/ontwerpkeuzes.md`, punt 8.
- [ ] Video van **max 30 s**, **Engelse ondertitels**, geen personen onder de 18, op YouTube (openbaar of niet-vermeld)
- [ ] Projectpagina in het Engels:
  - [ ] Project Name
  - [ ] High-Level Summary
  - [ ] Link to Project Demo
  - [ ] Link to Final Project (GitHub)
  - [ ] Detailed Project Description (ontwerpkeuzes, stroombudget, hardware en software)
  - [ ] Use of SI
  - [ ] NASA Data (tekst staat in `docs/nasa-std-3001.md`)
  - [ ] References: alle libraries, datasheets en NASA-documenten
- [ ] ⚠️ **Beide repo's openbaar** en alle links getest in een privévenster
- [ ] Documentatie in het Engels, of een Engelse samenvatting in de README
- [ ] Vinkjes "Terms and Conditions" en "Confirm All Work is Original"
- [ ] ⚠️ **Submit for Judging vóór 22:00** (deadline 23:59)
- [ ] Certificaat downloaden
