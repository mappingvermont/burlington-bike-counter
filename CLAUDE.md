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

## Display Remote Access (Web Workflow)

The Matrix Portal ESP32-S3 is sealed inside the fence enclosure with no USB access. `display/settings.toml` (gitignored, never committed — contains WiFi + web workflow credentials) enables CircuitPython's Web Workflow so `code.py` can be edited without opening the case:

- **Station mode** (current setup): the ESP32-S3 joins an existing WiFi network via `CIRCUITPY_WIFI_SSID` / `CIRCUITPY_WIFI_PASSWORD`. Reach the device at `http://circuitpython.local/` (password is `CIRCUITPY_WEB_API_PASSWORD`) from any device on the same network. Used at home during maintenance, joining `TheHousehold-2.4` — the ESP32-S3 radio is **2.4GHz only**, so it must join the 2.4GHz band specifically, not a 5GHz-only SSID.

Since `settings.toml` is gitignored, it must be copied onto the `CIRCUITPY` drive by hand (there's no way to add it later without physical USB access — do this before sealing the enclosure). The actual credential values live only in the local `display/settings.toml` file, not in git.
