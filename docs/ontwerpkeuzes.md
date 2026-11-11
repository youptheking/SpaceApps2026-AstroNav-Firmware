# Ontwerpkeuzes – polsstation

Dit document legt per keuze vast welke opties er waren, wat we kozen en waarom.
Uitgangspunt van het hele ontwerp: **dit is een proof of concept voor een hackathon van 2 dagen.** Het moet laten zien dat het probleem op te lossen is, niet dat elk onderdeel het beste is wat er bestaat. Waar we een concessie doen, staat erbij wat een productieversie op een eigen print anders zou doen.

Randvoorwaarden: draagbaar om de pols (dus klein en licht), een beperkt budget, en alle onderdelen moeten binnen 3 dagen leverbaar zijn (bij voorkeur TinyTronics).

---

## 1. Temperatuur: ingebouwde sensor van de MAX30102 in plaats van een losse sensor

**Opties**

| Sensor | Nauwkeurigheid | Kosten | Ruimte |
|---|---|---|---|
| Ingebouwd in de MAX30102 | ±1 °C, resolutie 0,0625 °C | €0 (zit er al in) | geen extra bordje |
| MCP9808 | ±0,25 °C typisch | €6,50 | ~19×13 mm |
| MAX30205 / TMP117 | ~0,1 °C rond 37 °C, gemaakt voor lichaamstemperatuur | ~€15 (Amazon) of niet leverbaar | klein bordje, wel extra |

**Keuze:** de ingebouwde sensor van de MAX30102.

**Gedachtegang**
- Eerst neigden we naar de MCP9808. Koorts begint ongeveer 1 °C boven je normale temperatuur, en dat is net zo groot als de afwijking van de MAX30102.
- Maar op de pols meet je hoe dan ook **huidtemperatuur**, niet de kerntemperatuur. Die wijkt een paar graden af, ook met een nauwkeurige sensor. Een absolute koortswaarde in graden kun je op de pols dus sowieso niet betrouwbaar geven.
- Wat je wél kunt meten, is de **afwijking ten opzichte van je eigen baseline**. Commerciële wearables doen het ook zo. Daarvoor telt vooral dat de waarde stabiel is en een fijne resolutie heeft. Absolute nauwkeurigheid is minder belangrijk, en de resolutie van 0,0625 °C is ruim voldoende.
- Een losse sensor kost een extra bordje op de pols, een extra I2C-apparaat, soldeerwerk en €6,50. Voor een proof of concept levert dat te weinig op.

**Concessie:** we tonen een **trend van de huidtemperatuur**, geen koorts in graden. App en pitch moeten dat ook zo formuleren.

**Aandachtspunt firmware:** de temperatuursensor zit naast de rode en infrarode LEDs, die de chip opwarmen. Lees de temperatuur daarom altijd uit met **dezelfde LED-stroom**, anders verschuift de baseline.

**Productieversie:** een speciale lichaamstemperatuursensor (MAX30205 of TMP117) op een eigen print, thermisch gekoppeld aan de huid en geïsoleerd van de rest van de elektronica.

**Pitch-zin:** *"For the prototype we use the MAX30102's built-in temperature sensor and track the deviation from the astronaut's own baseline. A production version would add a dedicated body-temperature sensor such as the MAX30205."*

---

## 2. CO2: SCD41 in plaats van SCD40 of SGP30

**Opties:** SGP30 (geschatte eCO2), SCD40 (echte CO2, gespecificeerd tot 2000 ppm, €18,25), SCD41 (echte CO2, gespecificeerd tot 5000 ppm, €26,50–29,50).

**Keuze:** SCD41.

**Gedachtegang**
- De SGP30 meet geen CO2, maar schat het op basis van vluchtige stoffen. Voor een alarm op verstikkingsgevaar is dat niet goed genoeg.
- In een ruimtestation ligt de CO2 structureel veel hoger dan in gebouwen op aarde. De SCD40 is maar gespecificeerd tot 2000 ppm en valt daar dus juist buiten. De SCD41 gaat tot 5000 ppm.
- De SCD41 heeft ook een single-shot-modus. Daarmee meet je af en toe in plaats van continu, en dat spaart de kleine accu.
- Dit is de duurste sensor in het ontwerp, maar ook de keuze die het meest specifiek aansluit op de omgeving van de missie. Daarom is dit de plek waar we bewust méér uitgeven.

**Concessie:** boven 5000 ppm geeft hij nog wel waarden, maar de nauwkeurigheid is dan niet gegarandeerd. Zeg dus "gespecificeerd tot 5000 ppm", niet "meet tot 5000 ppm". Het blijft een sensor voor consumentengebruik, geen ruimtevaartwaardige hardware.

**Pitch-zin:** *"We deliberately chose the SCD41 over the cheaper SCD40: CO2 on a space station sits far above typical indoor levels on Earth, and the SCD41 is specified up to 5000 ppm instead of 2000."*

---

## 3. ECG: plakelektrodes op de borst in plaats van koperen vlakken op de pols

**Opties**
1. Twee droge elektrodes op één pols
2. Eén elektrode op de pols plus een vlak op de rand dat je met een vinger van de andere hand aanraakt (zoals smartwatches dat doen)
3. Drie plakelektrodes op de borst, met een kabel naar het polsstation

**Keuze:** optie 3.

**Gedachtegang**
- Optie 1 werkt natuurkundig niet. Een ECG meet het spanningsverschil over het hart, en twee punten op dezelfde pols zitten vrijwel op dezelfde plek in dat veld.
- Optie 2 werkt, maar alleen als de astronaut actief een vinger op de rand legt. De eis is juist: **continu meten met zo min mogelijk aandacht van de astronaut.**
- Optie 3 meet continu. Dit doen astronauten in de praktijk ook voor continue hartbewaking, bijvoorbeeld met een Holter-monitor of de Astroskin/Bio-Monitor-shirt.
- De leads-off-detectie van de AD8232 ziet automatisch wanneer een elektrode loslaat, zodat het OLED dat kan melden.

**Concessie:** er loopt een kabel van de borst naar de pols, en plakelektrodes zijn wegwerp (ongeveer een dag per set). Bij beweging wordt het signaal rommelig.

**Redundantie:** de hartslag komt uit twee onafhankelijke bronnen. De MAX30102 (optisch, PPG) meet continu hartslag en HRV, de ECG voegt ritme en aritmie toe. Valt de een weg of ruist hij, dan blijft de ander werken.

---

## 4. GSR: zelfbouw in plaats van een Grove-module

**Keuze:** twee gefreesde koperen vlakken aan de onderkant, met een weerstand als spanningsdeler naar een ADC-pin.

**Gedachtegang:** de Grove GSR is groot (20×40 mm plus vingerbandjes), relatief duur (~€15,60) en alleen via Amazon te krijgen. Een spanningsdeler meet hetzelfde principe (huidgeleiding) en is voor een demo voldoende.

**Concessie:** minder versterking en filtering dan bij een module. Vertin het koper tegen oxidatie, en houd de GSR-vlakken los van alle andere contacten.

---

## 5. Formaat: kleinere display en accu

- **0.91" OLED (128×32) in plaats van 0.96":** een smal strookje past beter op een pols en is goedkoper. Er passen twee regels grote tekst op, genoeg voor korte instructies als "Neem rust" of "CO2 hoog".
- **LiPo van 250 mAh in plaats van 500 mAh:** kleiner, en de 500 mAh was niet op tijd leverbaar. Bij een geschat verbruik van 70–90 mA gaat hij ongeveer 3 uur mee, genoeg voor een demo. Met de single-shot-modus van de SCD41 langer.
- **Grootste bordje:** de AD8232 (35,5×30,5 mm). Een kleinere breakout bestaat niet. Op een eigen print zou de chip direct op de draagprint komen.

---

## 6. Inkoop: 1 set met reserves in plaats van 2 complete sets

**Gedachtegang:** twee sets kosten ~€175–185. Eén set met reserves alleen voor de goedkope onderdelen die makkelijk stukgaan of problemen geven (XIAO, MAX30102, OLED) kost ~€100. Op zaterdag kun je niets meer bijbestellen, dus voor die drie is de reserve een goedkope verzekering. Het prototype bouwen we op het breadboard, en daarna gaan dezelfde modules op female headers naar de draagprint.

---

## Samenvatting voor de pitch

Het polsstation laat met breakout-bordjes zien dat het concept werkt. De keuzes zijn gemaakt op **draagbaarheid, leverbaarheid en wat een proof of concept moet bewijzen.** Bewust méér uitgegeven: de CO2-sensor, omdat die direct samenhangt met de omgeving van de missie. Bewust bezuinigd: temperatuur en GSR, met een helder pad naar een productieversie op een eigen print.
