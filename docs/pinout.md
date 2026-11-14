# Pinindeling – XIAO ESP32-S3

Deze indeling volgt het schema `Nasa_Spaceapps_2026` (14 november 2026).

De XIAO ESP32-S3 heeft 11 GPIO-pinnen (D0–D10). Daarvan kunnen er 9 als ADC werken: D0–D5 en D8–D10 zitten allemaal op ADC1, die naast BLE gewoon werkt. D6 en D7 hebben geen ADC.

> **Let op bij de firmware:** de netnamen in het schema (`GPIO_0`, `GPIO_7`, …) volgen de **D-nummers** van de XIAO, **niet** de echte GPIO-nummers van de ESP32-S3. Gebruik in Arduino daarom altijd de macro's `D0`…`D10`, nooit kale getallen. Voorbeeld: `pinMode(D8, …)` is goed. `pinMode(8, …)` stuurt GPIO8 aan, en dat is pin **D9**.

| Pin (XIAO) | Netnaam in schema | GPIO (ESP32-S3) | Functie | Richting | Opmerking |
|---|---|---|---|---|---|
| D0 | GPIO_0 | 1 | AD8232 OUTPUT (ECG) | ADC in | Samplen op 250–500 Hz met een hardwaretimer |
| D1 | GPIO_1 | 2 | AD8232 LO− | digitaal in | Hoog = elektrode los |
| D2 | GPIO_2 | 3 | AD8232 LO+ | digitaal in | Hoog = elektrode los. GPIO3 is een strapping-pin, maar alleen relevant als de JTAG-efuse is gezet (standaard niet). |
| D3 | GPIO_3 | 4 | AD8232 SDN | digitaal uit | Laag = AD8232 uit. Niet gebruikt: `pinMode(D3, INPUT)`, dan houdt de pull-up op het bordje hem hoog. |
| D4 | I2C_SDA | 5 | I2C SDA | I2C | MAX30102, MPU-6050, BH1750, SCD41, OLED, BMP180 |
| D5 | I2C_SCL | 6 | I2C SCL | I2C | |
| D6 | – | 43 | **vrij laten** | – | UART0 TX. Bij het opstarten zendt de ROM hier een bootlog uit. |
| D7 | GPIO_7 | 44 | Piezobuzzer (via 100 Ω) | PWM uit | UART0 RX is bij het opstarten een ingang, dus de buzzer blijft stil tijdens het starten |
| D8 | GPIO_8 | 7 | Accuspanning (2× 200 kΩ, 220 nF) | ADC in | `analogReadMilliVolts(D8) * 2` |
| D9 | GPIO_9 | 8 | GSR (220 kΩ naar GND, 220 nF) | ADC in | |
| D10 | – | 9 | vrij | – | Reserve, bijvoorbeeld voor MAX30102 INT of MPU-6050 INT |

**In gebruik: 10 van de 11.** D10 is reserve. D6 blijft bewust vrij.

## Niet aangesloten (bewust)

- **MAX30102 INT** en **MPU-6050 INT:** de firmware pollt via I2C. Wil je later wrist-raise met de MPU-6050, dan gaat de INT naar D10.
- **AD8232 RA/LA/RL-pads:** de elektrodes komen via de 3,5 mm-jack.
- **I2C-pull-ups R5/R6:** staan in het schema als reserve ("DONT SOLDER"). Plaats ze alleen als je meet dat de modules samen te weinig pull-up geven. Doel: 2–5 kΩ totaal.

## Aandachtspunten

- **I2C-adressen** (geen conflicten): 0x23 (BH1750), 0x3C (OLED), 0x57 (MAX30102), 0x62 (SCD41), 0x68 (MPU-6050), 0x77 (BMP180).
- **USB-serieel:** de XIAO gebruikt de ingebouwde USB-poort voor serial en programmeren, niet D6/D7. Daarom kun je D7 als GPIO gebruiken.
- **Strapping-pinnen van de ESP32-S3** zijn GPIO0, 3, 45 en 46. Alleen GPIO3 (D2) ligt naar buiten, en die is hier als ingang veilig gebruikt.
