# Stroombudget – 1S LiPo 250 mAh

> **Dit zijn schattingen.** Ze komen uit datasheets en typische waarden, behalve waar "datasheet" staat. Meet het echte verbruik zaterdag met een multimeter in serie met de accu, of met een USB-stroommeter. Het grootste onzekere getal is de ESP32-S3 met BLE, en dat hangt sterk af van de firmware.

## Verbruik per onderdeel (bij 3,3 V)

| Onderdeel | Niet geoptimaliseerd | Geoptimaliseerd | Hoe je het verlaagt |
|---|---|---|---|
| XIAO ESP32-S3 met BLE | ~60 mA (40–100) | ~25 mA (15–35) | CPU op 80 MHz, light sleep tussen metingen, BLE-pakketten bundelen (bijv. 10× per seconde in plaats van per sample), Wi-Fi uit |
| SCD41 | 15 mA (datasheet, periodiek elke 5 s) | 1,5 mA (single shot elke 60 s: 0,20 mA + 77 mC/60 s, datasheet) | Low-power-periodiek (elke 30 s) = 3,2 mA. Single shot elke 5 min ≈ 0,5 mA. |
| 0.91" OLED | ~10 mA (5–20, afhankelijk van hoeveel pixels branden) | ~1 mA | Alleen aan bij een melding of een tik, daarna uit. Grote tekst, weinig witte pixels. |
| MAX30102 | ~3 mA (1–10) | ~2 mA | LED-stroom zo laag als nodig is voor een goed signaal, sample rate 100 Hz |
| MPU-6050 | ~4 mA (accel + gyro, datasheet 3,9 mA) | ~0,5 mA | Gyro uit als hij niet nodig is, alleen de accelerometer voor activiteit en slaap |
| AD8232 | ~0,2 mA (datasheet 170 µA) | ~0,2 mA | – |
| BH1750 | ~0,1 mA | ~0,1 mA | Eenmalige meetmodus |
| Power-LEDjes op de breakouts | ~5 mA (ca. 4× 1–2 mA) | 0 mA | **LED-weerstandjes eraf solderen.** Kost 5 minuten en scheelt 5 mA. |
| Spanningsdelers (GSR, accu) | <0,02 mA | <0,02 mA | – |
| Buzzer | alleen bij een alarm | alleen bij een alarm | – |
| **Totaal** | **~100 mA** | **~30 mA** | |

## Hoe lang gaat de accu mee?

Bruikbare capaciteit: ongeveer **200 mAh** van de 250 mAh. De XIAO gebruikt een lineaire regelaar, dus de stroom uit de accu is ongeveer gelijk aan de stroom op 3,3 V. Onder ~3,5 V zakt de 3,3 V-lijn weg, en een goedkope cel levert vaak iets minder dan opgegeven.

| Scenario | Verbruik | Accuduur |
|---|---|---|
| Niet geoptimaliseerd, alles continu | ~100 mA | **~2 uur** |
| Realistische hackathonfirmware (SCD41 low-power, power-LEDs eraf, de rest standaard) | ~80 mA | **~2,5 uur** |
| Geoptimaliseerd | ~30 mA | **~6–7 uur** |

**Conclusie:** voor een demo is het genoeg, zeker als je bij het filmen gewoon de USB-kabel aansluit. Voor een volledige werkdag van een astronaut (8+ uur) is 250 mAh te weinig. In een productieversie zou je een grotere cel (500–1000 mAh) en zuiniger firmware gebruiken, of een zuinigere controller zoals de nRF52840.

De twee grootste verbruikers zijn de **ESP32 met BLE** en de **SCD41**. Daar zit ook de meeste winst.

## Piekstroom: let op

- De SCD41 trekt bij een meting kort **175 mA (typisch) tot 205 mA (max)** (datasheet). De ESP32 heeft bij het zenden ook pieken.
- Samen kan dat even oplopen tot **~300 mA**. Voor een cel van 250 mAh is dat meer dan 1C. Kort mag dat, maar de spanning zakt dan in, en de ESP32 kan dan resetten (brownout).
- **Oplossing:** zet een buffercondensator van **100–220 µF** dicht bij de SCD41, op de 3,3 V-lijn, plus 10 µF en 100 nF keramisch. Krijg je toch resets bij elke CO2-meting, dan is dit de oorzaak.

## Opladen

**Een aparte lader is niet nodig.** De XIAO ESP32-S3 heeft een laadchip voor één LiPo-cel ingebouwd:

- De accu soldeer je aan de **BAT+/BAT−-pads aan de onderkant** van de XIAO.
- Opladen gaat via **dezelfde USB-C-poort waarmee je programmeert.** Is de USB aangesloten, dan draait alles op USB en laadt de accu tegelijk.
- **Laadstroom:** volgens de bronnen 50 of 100 mA, afhankelijk van de versie. Beide zijn veilig voor 250 mAh (≤0,5C). Reken op **3 tot 6 uur** van leeg naar vol. Laad hem dus vrijdagavond al op.
- Een rood LEDje op de XIAO knippert tijdens het laden.

**Gevolgen voor het ontwerp**

- **Behuizing:** houd een opening vrij voor de USB-C-poort. Daar laad én programmeer je.
- **Aan/uit-schakelaar:** zit hij tussen de accu en de BAT+-pad, dan laadt de accu ook niet als de schakelaar uit staat. Dat is acceptabel ("zet aan om te laden"), maar weet het wel. Op USB werkt alles ook met de schakelaar uit.
- **Accu-percentage:** de spanningsdeler van 2× ~200 kΩ naar een ADC-pin (zie de inkooplijst) geeft de accuspanning. Daaruit bereken je een percentage voor het OLED en het dashboard.
- **Beveiliging:** controleer of de LiPo een beveiligingsprint (PCM) heeft. De XIAO-lader beschermt waarschijnlijk niet tegen diepontlading. Zorg dat de firmware bij ~3,4 V een waarschuwing geeft en bij ~3,3 V in deep sleep gaat.
