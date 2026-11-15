# AstroNav – project status (English)

*State on Sunday 15 November 2026. Written for the Space Apps project page, so it is in English. Details per part are in the Dutch READMEs and `docs/`.*

**Challenge:** Create Health Monitoring Software for Astronauts on Space Missions
**Team:** Team AstroNav, solo
**Repos:** `SpaceApps2026-AstroNav-Firmware` (wrist station) · `SpaceApps2026-AstroNav-Dashboard` (desktop app)

## What it is

A wrist station that measures the astronaut's body and the cabin air, warns on the wrist even without a laptop, and streams everything over Bluetooth Low Energy to a desktop console that turns it into trends and concrete advice. Alarm limits for the cabin come from **NASA-STD-3001 Volume 2**.

## What works

### Wrist station (hardware + firmware 1.0)

| Part | Status |
|---|---|
| Custom PCB, milled on a desktop CNC, Seeed XIAO ESP32-S3 | Works |
| **MAX30102** optical sensor: heart rate, SpO2, perfusion, PPG at 100 Hz, chip temperature as skin-temperature trend | Works. Skin-contact detection now has hysteresis so readings no longer drop out on every small dip (fix built, being tested). |
| **AD8232** ECG at 250 Hz from three chest electrodes, R-peaks, RR intervals, loose-electrode detection | Works |
| **MPU-6050**: activity level, tremor (4–12 Hz), shock, double tap to acknowledge an alert | Works |
| **GSR** skin conductance (stress) | Works |
| **SCD41** real CO2, cabin temperature and humidity | Works |
| **BMP180** cabin pressure, pressure trend for leak detection, pressure-corrected ppCO2 | Works |
| **OLED** (0.96", portrait) with values, alerts and messages from the dashboard | Works |
| **Buzzer** with its own pattern per severity (info, warning, alarm) | Works |
| **Local alarms** without a laptop: CO2 over the NASA limit, low SpO2, high/low heart rate, electrode loose, no skin contact, shock, pressure drop, cabin climate | Works |
| **Bluetooth Low Energy** (own binary protocol v1), unlock with the 6-digit code on the watch screen | Works |
| **7-day history** of minute summaries in flash | Works on the watch |
| Self-recovery: missing sensors are retried every 10 s, I2C bus recovery, watchdog | Works |

### Desktop console (Tauri 2 + Rust + React)

| Part | Status |
|---|---|
| Connect and unlock the watch, automatic reconnect | Works |
| Caution & warning lamps per parameter, ISS-style classes | Works |
| Mute the alarm sound per lamp (lamp stays lit) | Works (new) |
| Live ECG trace, heart rate (ECG and optical), SpO2, HRV against personal baseline, skin-temperature trend | Works |
| Signal-quality checks: optical values during movement or with weak pulse are not shown as real; short dropouts hold the last good value | Works |
| Cabin air: 1-hour ppCO2 against the **NASA-STD-3001 3 mmHg limit**, pressure trend, temperature, humidity | Works |
| Body-system overview with what changes in space and when to act | Works |
| "What to do" advice, sent to the wrist with one click | Works |
| Event log with who raised and who handled each alert | Works |
| Demo watch on the laptop (same packets, 5-minute story) | Works |
| 44 automated tests (packets, signal quality, demo, interface) | Pass |

## How we used NASA resources

- **NASA-STD-3001 Vol. 2, Rev. F, [V2 6004]:** 1-hour average ppCO2 ≤ 3 mmHg. The watch converts the SCD41 ppm to partial pressure with the measured cabin pressure and alarms on the 1-hour average. The console shows the limit and the 2.5 mmHg long-term target.
- **[V2 6022]:** alert the crew locally and remotely, including on cabin pressure. Our architecture is exactly that: buzzer and OLED on the wrist, plus the console.
- **[V2 6012]:** cabin temperature and humidity limits.
- **[V2 6001]:** keep data for trend analysis: the 7-day history on the watch.
- **NASA OCHMO technical briefs** (CO2, ECLSS, Behavioral Health) for the reasoning behind the limits and why stress is monitored.

Physiological limits (SpO2, heart rate, stress) are not in the NASA documents we searched. They come from the WHO pulse-oximetry manual or are our own choice, and are labelled that way. The system gives advice, not a diagnosis.

## Honest limitations

| Limitation | Why |
|---|---|
| No battery in the prototype | Connector and switch broke during the build; the watch runs on USB. Battery values in demo mode are simulated and marked as such. |
| No light sensor | The PCB footprint pin order was wrong. |
| Skin temperature is a trend, not a fever reading | Comes from the MAX30102 chip temperature (±1 °C). |
| Wrist SpO2 is less reliable than a fingertip | The MAX30102 is designed for fingertips. |
| No ppO2 sensor | V2 6022 lists it; we do not cover it. |
| Check-in (sleep, mood, blood pressure, glucose) and crew name | Example data in the console. |
| 7-day watch history not yet shown in the console | Stored on the watch, not fetched yet. |
| Blood-pressure trend (pulse transit time) | Not built; raw ECG and PPG already share one clock so it can be added. |

## Built with

Seeed XIAO ESP32-S3 · MAX30102 · AD8232 · MPU-6050 · SCD41 · BMP180 · SSD1306 OLED · PlatformIO (Arduino core) · Tauri 2 · Rust (`btleplug`) · React · TypeScript · Vite · Vitest · desktop CNC (PCB) · 3D printing (enclosure)

## Use of AI

The idea and concept are our own and were fixed before any AI was used. Claude (Anthropic) was used to research parts, do power and pin calculations, look up limits in NASA-STD-3001, plan the weekend, draft documentation, and write most of the code from our own design (protocol, alarm logic, limits, layout). All of it was reviewed and tested on our own hardware. AI-assisted commits are marked in the git history.
