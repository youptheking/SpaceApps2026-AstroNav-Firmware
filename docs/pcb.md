# Draagprint – wat er op de PCB moet

De sensoren zijn allemaal kant-en-klare modules. De PCB verbindt ze en bevat een handvol losse onderdelen. Zie [pinout.md](pinout.md) voor de pintoewijzing en [stroombudget.md](stroombudget.md) voor de achtergrond bij de voeding.

## 1. Modules (op female headers, zodat je ze kunt wisselen)

| Module | Aansluitingen |
|---|---|
| XIAO ESP32-S3 | 2× 1×7 header, plus BAT+/BAT− aan de onderkant (draadjes) |
| MAX30102 | VIN, GND, SDA, SCL. INT niet aangesloten (reserve: D10). RD/IRD los laten. **Onderkant van het horloge, tegen de huid.** |
| MPU-6050 (GY-521) | VCC, GND, SDA, SCL. INT niet aangesloten (reserve: D10). AD0, XDA, XCL los laten. |
| AD8232 | 3.3V, GND, OUTPUT → D0, LO− → D1, LO+ → D2, SDN → D3. **Jack bereikbaar houden**, met trekontlasting voor de kabel. |
| GY-SCD41 | VCC, GND, SDA, SCL. **Bij het ventilatierooster aan de zijkant.** |
| 0.91" OLED | VCC, GND, SDA, SCL. **Bovenkant.** |
| BH1750 | VCC, GND, SDA, SCL. ADDR los (adres 0x23). **Bovenkant, met een venster in de behuizing.** |
| BMP180 | VCC (**3,3 V, niet 5 V**), GND, SDA, SCL. Optioneel, pas als de kern werkt. |

## 2. Losse onderdelen op de PCB

| Schakeling | Onderdelen | Verbinding |
|---|---|---|
| **I2C-bus** | – | SDA (D4) en SCL (D5) naar alle I2C-modules, plus 3,3 V en GND |
| **Voeding** | Schuifschakelaar (THT-footprint tussen BAT+ en BAT_SW+) | LiPo+ (BAT+) → schakelaar → BAT_SW+ van de XIAO. LiPo− → BAT− / GND. Stroom meten: zet de schakelaar uit en de multimeter (mA) over de twee pinnen. |
| **Accuspanning** | 2× ~200 kΩ + 0,22 µF | **BAT_SW+** (ná de schakelaar) → 200 kΩ → **D8** → 200 kΩ → GND. Condensator van D2 naar GND (min-kant naar GND), dan leest de ADC rustiger. |
| **GSR** | 220 kΩ + 0,22 µF + 2 koperen vlakken | 3,3 V → vlak GSR1 · huid · vlak GSR2 → **D9**. Vanaf D9: 220 kΩ naar GND en de condensator naar GND. |
| **Buzzer** | 100 Ω + piezo | **D7** → 100 Ω → piezo → GND (niet D6: die piept bij het opstarten) |
| **Buffer voor de SCD41** | 100–220 µF elco + 10 µF | Tussen 3,3 V en GND, **zo dicht mogelijk bij de SCD41**. Let op de polariteit. |
| **Testpunten** | Stukjes kaal koper of pinnetjes | GND, 3,3 V, SDA, SCL. Daar zet je je multimeter of logic analyzer op tijdens het debuggen. |

**Stroom meten zonder extra circuit:** zet de schakelaar uit en zet je multimeter (stand mA) over de twee pinnen van de schakelaar. De stroom loopt dan door de meter, en je meet één keer het echte verbruik.

## 3. Onderkant (huidkant)

```
   ┌─────────────────────────────┐
   │  ███        ▣▣         ███  │
   │  ███     MAX30102      ███  │
   │  GSR1                  GSR2 │
   └─────────────────────────────┘
       ←──── ~20 mm ────→
```
- GSR-vlakken: ~8×12 mm, minimaal 5 mm tussenruimte, **vertind**, 0,5–1 mm uitstekend.
- Al het andere koper aan de onderkant isoleren met lak of kaptontape. **Alleen de GSR-vlakken en het venster van de MAX30102 raken de huid.**

## 4. Behuizing

- Opening voor **USB-C** (laden en programmeren)
- **U.FL-antenne** van de XIAO niet onder de accu of tegen koper
- **Holte met een geluidsgat** (2–3 mm) boven de piezo
- **Rooster** aan de zijkant voor de SCD41
- **Venster** boven de BH1750 en het OLED
- Lussen voor het klittenband

## 5. Uit eigen voorraad

- Weerstanden: 100 Ω, 220 kΩ, 2× ~200 kΩ (100–470 kΩ werkt ook voor GSR en de accudeler)
- Female en male headers
- Dun siliconendraad, soldeertin, kaptontape

## Volgorde

1. **Eerst alles op het breadboard** volgens deze tabel. Pas als alles werkt, frees je de PCB.
2. Meet vóór het frezen de I2C-pull-ups (doel: 2–5 kΩ totaal) en de spanning op SDA (moet ~3,3 V zijn).
3. Houd de PCB **eenzijdig waar het kan**. Elke via die je zelf moet maken, kost tijd.
