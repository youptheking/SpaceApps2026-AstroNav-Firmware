# Pinindeling – XIAO ESP32-S3

De XIAO ESP32-S3 heeft 11 GPIO-pinnen (D0–D10). Daarvan kunnen er 9 als ADC werken: D0–D5 en D8–D10 zitten allemaal op ADC1, die naast BLE gewoon werkt. D6 en D7 hebben geen ADC.

**Nodig: 8 pinnen. Optioneel: 3. Totaal: 11 van de 11.**

| Pin | GPIO | Functie | Richting | Nodig? | Opmerking |
|---|---|---|---|---|---|
| D0 | 1 | AD8232 OUTPUT (ECG) | ADC in | ja | Samplen op 250–500 Hz |
| D1 | 2 | GSR (spanningsdeler) | ADC in | ja | |
| D2 | 3 | Accuspanning (2× ~200 kΩ) | ADC in | ja | GPIO3 is een strapping-pin, maar alleen relevant als de JTAG-efuse is gezet (standaard niet). Een hoogohmige deler is hier veilig. |
| D3 | 4 | AD8232 LO− | digitaal in | ja | Leads-off-detectie |
| D4 | 5 | I2C SDA | I2C | ja | MAX30102, MPU-6050, SCD41, OLED |
| D5 | 6 | I2C SCL | I2C | ja | |
| D6 | 43 | AD8232 SDN (shutdown) | digitaal uit | optioneel | Dit is UART0 TX. Bij het opstarten zendt de ROM hier een bootlog uit. Daarom alleen een **ingang van een ander onderdeel** op deze pin, nooit een uitgang (die zou ertegen in sturen) en nooit de buzzer (die zou piepen bij het opstarten). SDN is een ingang van de AD8232, dus dat is veilig. Kort in- en uitschakelen bij het opstarten is onschuldig. |
| D7 | 44 | AD8232 LO+ | digitaal in | ja | UART0 RX is bij het opstarten een ingang, dus er stuurt niets tegenin |
| D8 | 7 | Piezobuzzer | PWM uit | ja | |
| D9 | 8 | MPU-6050 INT | digitaal in | optioneel | Pols optillen of tikken zet het OLED aan, en wekt de chip uit sleep. Vervangt een knop. |
| D10 | 9 | MAX30102 INT | digitaal in | optioneel | "Data klaar". Zonder deze pin kun je ook pollen via I2C. |

## Als je pinnen tekortkomt

In volgorde van wat je het eerst laat vallen:
1. **MAX30102 INT (D10):** pollen via I2C werkt ook prima.
2. **AD8232 SDN (D6):** zonder deze pin staat de AD8232 altijd aan, wat maar ~0,2 mA kost.
3. **Eén van LO+ / LO−:** één van de twee is genoeg om te zien dat een elektrode los is. Je weet dan alleen niet welke.

Een **knop** is niet nodig: wrist-raise of tikken via de MPU-6050 (D9) zet het OLED aan.

## Aandachtspunten

- **Geen extra I2C-adressen nodig:** alle I2C-apparaten delen D4/D5. Adressen: 0x3C (OLED), 0x57 (MAX30102), 0x62 (SCD41), 0x68 (MPU-6050).
- **USB-serieel:** de XIAO gebruikt de ingebouwde USB-poort voor serial en programmeren, niet D6/D7. Daarom kun je D6/D7 als GPIO gebruiken.
- **Strapping-pinnen van de ESP32-S3** zijn GPIO0, 3, 45 en 46. Alleen GPIO3 (D2) ligt naar buiten, en die is hier veilig gebruikt.
