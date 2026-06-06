# Burlington Bike Counter

DIY pneumatic road tube bike counter. Silicone tube across a bike lane detects tire pressure pulses; an nRF52840 counts front/rear wheel pairs and BLE-advertises the running total to an LED display.

## Hardware Stack

3-board Feather stack (bottom to top):
1. **Feather nRF52840** — main MCU, BLE, analog read on A0
2. **DS3231 RTC FeatherWing** — real-time clock over I2C, CR1220 coin cell backup
3. **FeatherWing Proto** — MPX5010DP pressure sensor + MicroSD breakout (CS on pin 10)

Display: Matrix Portal ESP32-S3 + 64×32 RGB LED panel, powered by USB power bank.

## Key Files

- `phase2/phase2.ino` — Arduino firmware for the nRF52840
- `display/code.py` — CircuitPython firmware for the Matrix Portal display
- `phase2/analyze.py` — analysis script for characterization data
- `next-steps.md` — current progress and remaining hardware tasks

## Detection Parameters

Derived from characterization test 2026-05-31:
- Noise floor: ~34 ADC, weakest real event peak: 68
- `THRESHOLD = 65` — ADC value a pulse must exceed
- `MIN_PULSE_MS = 5` — minimum pulse duration to count
- `MIN_PAIR_GAP = 200` / `MAX_PAIR_GAP = 1500` — front/rear wheel timing window (ms)

## SD Logging

Appends to `counts.csv` on each power cycle. `bikeCount` resets to 0 each session but timestamps allow per-session reconstruction. CSV columns: `datetime,event,peak,count`.

## BLE Format

Manufacturer data, company ID `0xFFFF`, count as uint16 little-endian. Display scans passively and re-renders on change.
