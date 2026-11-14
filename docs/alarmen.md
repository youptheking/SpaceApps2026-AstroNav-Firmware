# Alarmen

Het polsstation controleert elke seconde alle metingen en geeft zelf een waarschuwing, ook als de laptop niet verbonden is. De code staat in `src/alerts.cpp`.

## Bronnen

| Bron | Wat we eruit gebruiken |
|---|---|
| [NASA-STD-3001 Volume 2, Revision F](https://standards.nasa.gov/standard/NASA/NASA-STD-3001_VOL_2) (goedgekeurd 2026-07-14, actief) | **[V2 6004]** §6.2.1.3 – CO2-grens<br>**[V2 6022]** §6.2.6.1 – lokaal en op afstand waarschuwen<br>**[V2 6012]** §6.2.3.1 – grenzen voor cabinetemperatuur en luchtvochtigheid |
| [NASA OCHMO Technical Brief – Carbon Dioxide](https://www.nasa.gov/wp-content/uploads/2023/12/ochmo-tb-004-carbon-dioxide.pdf) | Waarom de grens op 3 mmHg ligt, en het doel van 2,5 mmHg |
| [NASA OCHMO Technical Brief – ECLSS](https://www.nasa.gov/wp-content/uploads/2023/07/eclss-technical-brief-ochmo.pdf) | Overzicht van V2 6004, V2 6012 en V2 6022 |
| [NASA OCHMO Technical Brief – Behavioral Health](https://www.nasa.gov/wp-content/uploads/2023/03/behavioral-health-technical-brief-ochmo-1.pdf) | Waarom stress bewaken belangrijk is |

De oudere [Revision D](https://standards.nasa.gov/sites/default/files/standards/NASA/D/nasa-std-3001-vol-2-rev-d-signature.pdf) staat er ook nog online. Wij hebben het in Revision F nagekeken: V2 6004 staat daar nog steeds op 3 mmHg, en V2 6022 is inhoudelijk hetzelfde gebleven.

### Wat de norm zegt

> **[V2 6004]** The system shall limit the average one-hour CO2 partial pressure (ppCO2) in the habitable volume to no more than 3 mmHg.

> **[V2 6022]** The system shall alert the crew locally and remotely when atmospheric parameters, including atmospheric pressure, humidity, temperature, ppO2, and ppCO2 are outside safe limits.

Uit de CO2-brief: tijdens ISS-missies kregen astronauten hoofdpijn bij 2,8–4,5 mmHg. Met de grens van 3 mmHg blijft de kans op hoofdpijn onder de 1,4 %. Om onder 1 % te blijven moet het 7-daagse gemiddelde onder 2,5 mmHg liggen.

## Hoe we de norm toepassen

**CO2 (V2 6004).** De SCD40 meet in ppm, de norm rekent in mmHg (partiële druk):

```
ppCO2 [mmHg] = ppm × cabinedruk [mmHg] / 1 000 000
```

We nemen een cabinedruk van 760 mmHg (1 atm, zoals op het ISS). Dan is 3 mmHg ≈ 3950 ppm en 2,5 mmHg ≈ 3290 ppm. Bij een andere cabinedruk pas je `CABIN_PRESSURE_MMHG` aan in `src/alerts.cpp`.

Net als de norm kijken we naar het **gemiddelde over het laatste uur** en niet naar losse metingen. Na het opstarten is het gemiddelde over de tijd die er tot dan toe is. Het 1-uursgemiddelde gaat ook mee in het telemetriepakket (`ppco2_avg`).

**Lokaal en op afstand waarschuwen (V2 6022).**
- Lokaal: de alarmtekst staat onderaan op de OLED, ook op het koppelscherm als er geen laptop is. Bij kritieke alarmen piept de buzzer elke 3 seconden tot `ack`.
- Op afstand: het alarm staat als bit in het telemetriepakket. Bij een nieuw of opgelost alarm wordt het pakket **meteen** verstuurd, niet pas bij het volgende interval.

**Cabineklimaat (V2 6012).** De norm geeft een werkgebied tussen 18 °C / 75 % RV en 27 °C / 25 % RV. Wij gebruiken de rechthoek eromheen (18–27 °C en 25–75 % RV), die iets ruimer is dan de figuur in de norm.

**Wat we niet dekken.** V2 6022 noemt ook cabinedruk en ppO2. Daarvoor zit er geen sensor op het polsstation.

## Alle alarmen

| Bit | Alarm | Grens | Moet zo lang kloppen | Buzzer | Bron |
|---|---|---|---|---|---|
| 4 | CO2 te hoog | 1-uursgemiddelde > 3 mmHg | 5 s | ja | NASA [V2 6004] |
| 8 | CO2 stijgt | 1-uursgemiddelde > 2,5 mmHg | 5 s | nee | OCHMO CO2-brief (doel 2,5 mmHg) |
| 9 | Cabineklimaat | < 18 of > 27 °C, of < 25 of > 75 % RV | 60 s | nee | NASA [V2 6012] |
| 0 | SpO2 laag | < 92 % | 10 s | ja | klinische grens, niet uit NASA-STD-3001 |
| 1 | Hartslag hoog | > 120 bpm in rust (activiteit < 30) | 30 s | nee | klinische grens, niet uit NASA-STD-3001 |
| 2 | Hartslag laag | < 40 bpm | 30 s | nee | klinische grens, niet uit NASA-STD-3001 |
| 3 | Temperatuur hoog | huid > 37,5 °C | 5 min | nee | klinische grens. Aan de pols alleen een indicatie. |
| 5 | Stress | huidgeleiding 25 % boven de eigen basislijn (~10 min) | 15 s | nee | eigen keuze, onderbouwd met de Behavioral Health-brief |
| 7 | ECG-elektrodes los | LO+ of LO− hoog, alleen met `ecg on` | 5 s | nee | – |
| 6 | Batterij bijna leeg | < 3,5 V | 10 s | nee | – |

Een alarm gaat uit zodra de waarde 5 seconden lang weer goed is.

Komen er meerdere alarmen tegelijk, dan staat op het scherm alleen het belangrijkste. De volgorde is: SpO2, CO2 te hoog, hartslag hoog, hartslag laag, temperatuur, CO2 stijgt, cabineklimaat, stress, elektrodes, batterij.

## Testen met de simulatie

Standaard blijft de gesimuleerde CO2 netjes onder de grens (1,1–2,4 mmHg). Elke 3 minuten komt er een zuurstofdip die het SpO2-alarm en het stressalarm laat afgaan. Om de andere alarmen te testen zet je een vaste waarde. `0` zet het veld weer op automatisch.

| Commando | Wat er gebeurt |
|---|---|
| `sim co2 4500` | 4500 ppm = 3,4 mmHg → na 5 s "CO2 > 3 mmHg", buzzer |
| `sim co2 3500` | 2,7 mmHg → "CO2 > 2.5 mmHg" |
| `sim spo2 88` | SpO2-alarm na 10 s, buzzer |
| `sim hr 140` | hartslag-alarm na 30 s |
| `sim cabin 29` | cabineklimaat-alarm na 60 s |
| `sim temp 38.2` | temperatuuralarm na 5 min |
| `ack` | buzzer stil voor de alarmen die nu actief zijn |

Bij `sim co2` begint het 1-uursgemiddelde opnieuw, anders duurt het een uur voordat je iets ziet.
