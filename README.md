# AstroNav Wrist Station – Firmware

**NASA Space Apps Challenge 2026** · Challenge: *Create Health Monitoring Software for Astronauts on Space Missions*
Team AstroNav (solo) · Amsterdam (virtual) · by [YoupSpace](https://youpspace.com)

A wrist-worn sensor station that measures the astronaut's body and the cabin air, warns on the wrist even without a laptop, and streams everything over Bluetooth Low Energy to the desktop console.

| Repo | What |
|---|---|
| **SpaceApps2026-AstroNav-Firmware** (this repo) | Wrist station: PCB, sensors, firmware |
| [SpaceApps2026-AstroNav-Dashboard](https://github.com/youptheking/SpaceApps2026-AstroNav-Dashboard) | Desktop console that receives the data and turns it into trends and advice |

> Separate from the other AstroNav projects (AstroNav-Nano and related repos).

## Features

- **Body:** heart rate and SpO2 (MAX30102, PPG at 100 Hz), ECG with R-peaks and RR intervals (AD8232, 250 Hz, chest electrodes), skin conductance for stress (GSR), skin-temperature trend
- **Movement:** activity, tremor (4–12 Hz), shock and double tap to acknowledge (MPU-6050). Based on motion, not posture, because in microgravity there is no gravity reference.
- **Cabin:** real CO2, temperature and humidity (SCD41), cabin pressure and leak alarm (BMP180), ppCO2 in mmHg corrected for the measured pressure
- **Local alarms without a laptop:** NASA-STD-3001 CO2 limit, low SpO2, high or low heart rate, loose electrode, shock, pressure drop, cabin climate. Shown on the OLED, each severity with its own buzzer pattern.
- **Messages from the console** to the wrist, such as "Take a break" or "Start breathing exercise"
- **Bluetooth Low Energy** with its own binary protocol: raw streams, vitals, air, cabin, status and events. No OS pairing: the app unlocks the watch with the 6-digit code on its screen, so it works on Windows, Android and `btleplug`.
- **7-day history** of minute summaries in flash
- **One clock for all fast sensors:** ECG, PPG, IMU and GSR on a fixed 4 ms tick, so RR intervals (and later pulse transit time) line up
- **Honest data:** optical values during movement or weak perfusion are dropped instead of shown. In demo mode simulated values are flagged in every status update and marked with `*` on the watch.
- **Self-recovering:** missing sensors are retried every 10 s, I2C bus recovery, watchdog

## Hardware

Seeed XIAO ESP32-S3 on a double-sided PCB milled on a desktop CNC, in a 3D-printed case with a velcro strap.

| Part | Bus | Position | Purpose |
|---|---|---|---|
| MAX30102 | I2C | Underside, on the skin | Heart rate, SpO2, perfusion, chip temperature |
| AD8232 | ADC | Cable to the chest | ECG, RR intervals, electrode-off detection |
| GSR pads | ADC | Underside, on the skin | Skin conductance (stress) |
| MPU-6050 | I2C | On the PCB | Activity, tremor, shock, double tap |
| SCD41 | I2C | Side, behind a grille | CO2, temperature, humidity |
| BMP180 | I2C | On the PCB | Cabin pressure |
| 0.96" OLED | I2C | Top | Values, alerts, messages |
| Piezo buzzer | PWM | On the PCB | Local alarm |

## Build and flash

```
pio run -e xiao_esp32s3 -t upload
```

USB serial commands (`info`, `sim`, `alert`, `ecgtest`, …) are listed in [docs/ble-protocol.md](docs/ble-protocol.md).

## NASA resources

The local alarms follow **NASA-STD-3001 Volume 2, Revision F**:

- **[V2 6004]** 1-hour average ppCO2 must stay at or below 3 mmHg
- **[V2 6022]** alert the crew locally (OLED and buzzer) and remotely (console), also on a pressure drop
- **[V2 6012]** cabin temperature and humidity limits
- **[V2 6001]** keep data for trend analysis: 7 days of minute summaries on the watch

Physiological limits (SpO2, heart rate) come from the WHO Pulse Oximetry Training Manual or are our own choice, and are labelled that way. The system gives advice, not a diagnosis. Details: [docs/alarmen.md](docs/alarmen.md) and [docs/nasa-std-3001.md](docs/nasa-std-3001.md).

## Limitations

| What | Why |
|---|---|
| No battery or power switch | Both broke off the PCB during the build; the watch runs on USB. The battery value in demo mode is simulated and flagged. |
| No light sensor (BH1750) | The footprint pin order was wrong. Light in demo mode is simulated and flagged. |
| Skin temperature is a trend, not a fever reading | Comes from the MAX30102 chip temperature |
| Wrist SpO2 is less reliable than a fingertip | The MAX30102 is designed for fingertips |
| No ppO2 sensor | Listed in V2 6022, not covered |
| Blood-pressure trend (PTT), sleep analysis | Not built. ECG and PPG already share one clock, so it can be added in the console. |

## Use of AI

Per the Space Apps rules, we disclose where AI ("SI") was used.

- **Idea and concept:** our own. The wrist station plus console and the choice of challenge were fixed before any AI was used.
- **Claude (Anthropic)** was used to research parts, do power and pin calculations, look up limits in NASA-STD-3001, plan the weekend, draft the documentation, and write most of the code from our own design (protocol, alarm logic, limits, screen layout).
- **Hardware** (PCB design, milling, soldering, case) and all testing on the real watch were done by hand. Everything AI produced was reviewed and tested on our own hardware. AI-assisted commits are marked in the git history.

## Documentation (Dutch)

- [docs/status.md](docs/status.md): project status in English
- [docs/sensoren.md](docs/sensoren.md): wiring, addresses, notes per sensor, hardware tests
- [docs/ble-protocol.md](docs/ble-protocol.md): BLE protocol v1, extensions, USB commands
- [docs/alarmen.md](docs/alarmen.md): alarms, limits, buzzer patterns, sources
- [docs/ecg-elektrodes.md](docs/ecg-elektrodes.md): where the ECG electrodes go
- [docs/ontwerpkeuzes.md](docs/ontwerpkeuzes.md): design choices
- [docs/pinout.md](docs/pinout.md), [docs/pcb.md](docs/pcb.md), [docs/stroombudget.md](docs/stroombudget.md), [docs/inkooplijst.md](docs/inkooplijst.md): pins, PCB, power budget, parts list

---

Made by [YoupSpace](https://youpspace.com) for NASA Space Apps 2026.
