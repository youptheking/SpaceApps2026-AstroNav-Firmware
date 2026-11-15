# Alarmen

Het polsstation controleert elke seconde alle metingen en waarschuwt zelf, ook als de laptop niet verbonden is. De code staat in `src/alerts.cpp`, de buzzerpatronen in `src/buzzer.cpp`. De codes zijn die van `EVENT` in [ble-protocol.md](ble-protocol.md).

## Bronnen

| Bron | Wat we eruit gebruiken |
|---|---|
| [NASA-STD-3001 Volume 2, Revision F](https://standards.nasa.gov/standard/NASA/NASA-STD-3001_VOL_2) (goedgekeurd 2026-07-14, actief) | **[V2 6004]** §6.2.1.3 – CO2-grens<br>**[V2 6022]** §6.2.6.1 – lokaal en op afstand waarschuwen, ook bij cabinedruk, temperatuur en luchtvochtigheid<br>**[V2 6012]** §6.2.3.1 – grenzen voor cabinetemperatuur en luchtvochtigheid<br>**[V2 6001]** §6.1 en Tabel 6.1-1 – data voor trendanalyse (zie de geschiedenis in [ble-protocol.md](ble-protocol.md#uitbreidingen-op-v1)) |
| [NASA OCHMO Technical Brief – Carbon Dioxide](https://www.nasa.gov/wp-content/uploads/2023/12/ochmo-tb-004-carbon-dioxide.pdf) | Waarom de grens op 3 mmHg ligt, en het doel van 2,5 mmHg |
| [NASA OCHMO Technical Brief – ECLSS](https://www.nasa.gov/wp-content/uploads/2023/07/eclss-technical-brief-ochmo.pdf) | Overzicht van V2 6004, V2 6012 en V2 6022 |
| [NASA OCHMO Technical Brief – Behavioral Health](https://www.nasa.gov/wp-content/uploads/2023/03/behavioral-health-technical-brief-ochmo-1.pdf) | Waarom stress bewaken belangrijk is (geeft geen grenswaarden) |
| WHO Pulse Oximetry Training Manual | SpO2-drempels van 94 % en 90 % |

De oudere [Revision D](https://standards.nasa.gov/sites/default/files/standards/NASA/D/nasa-std-3001-vol-2-rev-d-signature.pdf) staat er ook nog online. Wij hebben het in Revision F nagekeken: V2 6004 staat daar nog steeds op 3 mmHg, en V2 6022 is inhoudelijk hetzelfde gebleven.

### Wat de norm zegt

> **[V2 6004]** The system shall limit the average one-hour CO2 partial pressure (ppCO2) in the habitable volume to no more than 3 mmHg.

> **[V2 6022]** The system shall alert the crew locally and remotely when atmospheric parameters, including atmospheric pressure, humidity, temperature, ppO2, and ppCO2 are outside safe limits.

Uit de CO2-brief: tijdens ISS-missies kregen astronauten hoofdpijn bij 2,8–4,5 mmHg. Met de grens van 3 mmHg blijft de kans op hoofdpijn onder de 1,4 %. Om onder 1 % te blijven moet het 7-daagse gemiddelde onder 2,5 mmHg liggen.

## Hoe we de norm toepassen

**CO2 (V2 6004).** De SCD41 meet in ppm, de norm rekent in mmHg (partiële druk):

```
ppCO2 [mmHg] = ppm × cabinedruk [Pa] / 1 000 000 / 133,322
```

De cabinedruk komt van de BMP180. Zonder BMP180 nemen we 1 atm (101 325 Pa, zoals op het ISS); dan is 3 mmHg ≈ 3950 ppm. Net als de norm kijken we naar het **gemiddelde over het laatste uur** en niet naar losse metingen. Na het opstarten is het gemiddelde over de tijd die er tot dan toe is. Het gaat mee in `AIR.ppco2_1h_c100`, met `nasa_status` 0/1/2 (onder 2,5 / boven 2,5 / boven 3 mmHg).

**Lokaal en op afstand waarschuwen (V2 6022).**
- Lokaal: de alarmtekst staat in de gele strook van het OLED, ook op het koppelscherm als er geen laptop is. De buzzer speelt het patroon van de ernst.
- Op afstand: elk alarm gaat **meteen** als `EVENT` naar de laptop (indicate, dus de ontvangst wordt bevestigd), en staat als bit in `STATUS.alarms`.

**Cabinedruk (V2 6022).** De BMP180 meet de druk elke seconde. De firmware legt een rechte lijn door de laatste minuut; daalt die sneller dan 50 Pa per minuut (3 kPa per uur), een minuut lang, dan is dat een mogelijk lek. Het weer verandert de druk hooguit een paar Pa per minuut. Die 50 Pa/min is een eigen keuze.

**Cabineklimaat (V2 6012).** De norm geeft een werkgebied tussen 18 °C / 75 % RV en 27 °C / 25 % RV. Wij gebruiken de rechthoek eromheen (18–27 °C en 25–75 % RV), iets ruimer dan de figuur in de norm. Dit is code 12, een uitbreiding op protocol v1.

**Wat we niet dekken.** V2 6022 noemt ook ppO2. Daarvoor zit er geen sensor op het polsstation.

**Lichaamswaarden.** In de doorzochte NASA-documenten staan geen alarmwaarden voor SpO2, hartslag of stress. Die komen uit algemene medische conventies of zijn een eigen keuze, en staan zo in de tabel. Het systeem geeft **adviezen, geen diagnoses**.

## Alle alarmen

| Code | Alarm | Grens | Zo lang | Ernst | Bron |
|---|---|---|---|---|---|
| 0 | CO2 boven de NASA-grens | 1-uursgemiddelde > 3 mmHg | direct | alarm | NASA [V2 6004] |
| 1 | CO2 nu boven 3 mmHg | actuele ppCO2 > 3 mmHg | 60 s | waarschuwing | vroege waarschuwing voor [V2 6004] |
| 2 | SpO2 laag | < 94 % = waarschuwing, < 90 % = alarm | 30 s | waarschuwing / alarm | WHO Pulse Oximetry Training Manual |
| 3 | Hartslag hoog | > 120 bpm **in rust** (IMU: activiteit 0) | 5 min | waarschuwing | eigen keuze (normale rusthartslag 60–100) |
| 4 | Hartslag laag | < 40 bpm | 1 min | waarschuwing | eigen keuze (bij getrainde mensen is 40–60 normaal) |
| 5 | ECG-elektrode los | LO+ of LO− hoog | 5 s | info | – |
| 6 | Geen huidcontact | IR-signaal van de MAX30102 te laag | 10 s | info | – |
| 7 | Schok | meer dan 3 g | direct | waarschuwing | eigen keuze |
| 8 | Drukdaling (mogelijk lek) | trend < −50 Pa/min | 60 s | alarm | NASA [V2 6022], grens eigen keuze |
| 9 | Accu bijna leeg | < 15 % | 10 s | waarschuwing | – |
| 10 | Hoge stress | huidgeleiding 25 % boven de eigen basislijn, en als de ECG er is ook RMSSD 30 % onder de eigen basislijn | 10 min | waarschuwing | eigen keuze, onderbouwd met de Behavioral Health-brief |
| 11 | Bevestigd door de astronaut | dubbeltik | – | info | – |
| 12 | Cabineklimaat (uitbreiding) | < 18 of > 27 °C, of < 25 of > 75 % RV | 60 s | waarschuwing | NASA [V2 6012] |

Een alarm gaat uit zodra de waarde 5 seconden lang weer goed is. De basislijnen van GSR en RMSSD volgen langzaam (~10 minuten), zodat alleen een verandering opvalt; voor HRV en huidgeleiding bestaat geen vaste grens voor iedereen.

Komen er meerdere alarmen tegelijk, dan staat op het scherm het ergste (alarm voor waarschuwing voor info), en bij gelijke ernst in deze volgorde: SpO2, CO2 1 uur, druk, CO2 nu, hartslag hoog, hartslag laag, schok, cabine, stress, accu, elektrode, huidcontact.

## Buzzer

| Ernst | Patroon |
|---|---|
| info | één korte piep (alleen bij een melding van het dashboard) |
| waarschuwing | 3× kort, elke 30 s opnieuw tot bevestigen |
| alarm | continu afwisselend hoog/laag tot bevestigen |

Een **dubbeltik** op het horloge bevestigt alles wat op dat moment actief is. Het dashboard kan één code stil zetten met `control` 0x03. Komt een alarm later opnieuw op, of wordt het erger, dan piept hij weer.

## Testen met de simulatie

Via USB: `sim on` vult ontbrekende sensoren met nepwaarden, `sim <veld> <waarde>` zet een vaste waarde (`0` = weer automatisch). Bij `sim co2` begint het 1-uursgemiddelde opnieuw, anders duurt het een uur voordat je iets ziet.

| Commando | Wat er gebeurt |
|---|---|
| `sim co2 4500` | 4500 ppm ≈ 3,4 mmHg: code 0 meteen, code 1 na 60 s |
| `sim spo2 92` | code 2 als waarschuwing na 30 s |
| `sim spo2 88` | code 2 als alarm na 30 s |
| `sim hr 140` | code 3 na 5 min (alleen als de IMU rust meldt) |
| `sim cabin 29` | code 12 na 60 s |
| `alert 10 1 0` | melding van het dashboard nadoen: "Take a break", waarschuwing |
| `ack` | hetzelfde als een dubbeltik |

Een echte sensor gaat altijd voor: `sim` vult alleen aan wat er niet is.
