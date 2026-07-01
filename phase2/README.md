# Phase 2 — Full Deployment

Order after Phase 1 confirms the sensor works. The Feather, sensor, and tubing carry over from Phase 1.

---

## BOM (~$83)

| Part | Source | Link | Price |
|---|---|---|---|
| LiPo battery 3.7V 6600mAh | Adafruit | [#353](https://www.adafruit.com/product/353) | $24.50 |
| DS3231 Precision RTC FeatherWing | Adafruit | [#3028](https://www.adafruit.com/product/3028) | $13.95 |
| FeatherWing Proto | Adafruit | [#2884](https://www.adafruit.com/product/2884) | $4.95 |
| MicroSD SPI breakout | Adafruit | [#4682](https://www.adafruit.com/product/4682) | $3.50 |
| PG-9 cable gland (buy 2) | Adafruit | [#761](https://www.adafruit.com/product/761) | $1.95 ea |
| Stacking Headers | DigiKey | [1528-2830-ND](https://www.digikey.com/en/products/detail/adafruit-industries-llc/2830/5823439) | ~$1.25 |
| CR1220 coin cell (Panasonic) | DigiKey | [CR1220](https://www.digikey.com/en/products/detail/panasonic-energy/CR1220/269740) | $1.00 |
| MicroSD card, 64GB | Amazon | — | $30.00 |

**Power:** LiPo 6600mAh. Dominant draw is the sensor (~4mA) plus Feather sleep current. Expected life ~34 days. Battery voltage readable via VBAT.

**Use a standard USB charger** — the panel mount cable has no Power Delivery resistors and won't work with Apple USB-C chargers.

---

## Board Architecture

**3-board stack (bottom to top):**
```
Feather nRF52840        ← female headers (already soldered)
DS3231 RTC FeatherWing  ← stacking headers #2830 (male pins down into Feather, female sockets up)
FeatherWing Proto       ← male headers (included) plug into RTC's female sockets
```

All Feather signals (3V, GND, A0, SCK, MOSI, MISO, pin 10, etc.) pass through the stacking headers and are accessible as labeled pads on the FeatherWing Proto's PCB surface.

**Sensor orientation:** pin 1 (VOUT, marked end) faces LEFT. See [phase1/README.md](../phase1/README.md) for the full pin diagram. Pins go left to right: 1, 2, 3, 4, 5, 6 in the y=13.97 proto row. Barbs point straight up away from the board (perpendicular to the pins).

---

## Proto Board Layout

The board is 9 holes wide (short axis). Long axis left-to-right, RST pin at the left end.

```
◉ = Feather header pin    ○ = duplicate IO pad (tied to ◉ same column)
║ = power rail hole (all connected within strip)    · = plain isolated proto hole

          RST    3V║    AREF   GND strip   A0     A1     A2     A3     A4     A5
x (mm):   6.35   8.89   11.43   13.97    16.51  19.05  21.59  24.13  26.67  29.21
         ──────  ─────  ──────  ─────────  ─────  ─────  ─────  ─────  ─────  ─────
TOP HDR     ·    3V║      ·      GND║       R1◉    R2◉    R3◉    R4◉    R5◉    R6◉   y=21.59
TOP DUP     ·    3V║      ·      GND║       R1○    R2○    R3○    R4○    R5○    R6○   y=19.05
         ──────  ─────  ──────  ─────────  ─────  ─────  ─────  ─────  ─────  ─────
GRID        ·    3V║      ·      GND║        ·      ·      ·      ·      ·      ·    y=16.51
GRID        ·  3V║←w3    ·   GND║←w2+C1  [1]+C1  [2]    [3]    [4]    [5]    [6]  y=13.97 ← SENSOR
GRID        ·  3V║────C3────GND║          ·      ·      ·      ·      ·      ·    y=11.43 ← C3: 1µF
GRID        ·  3V║────C2────GND║          ·      ·      ·      ·      ·      ·    y=8.89  ← C2: 0.01µF
GRID        ·    3V║      ·      GND║        ·      ·      ·      ·      ·      ·    y=6.35
         ──────  ─────  ──────  ─────────  ─────  ─────  ─────  ─────  ─────  ─────
BOT DUP   RST○  3V○←SD  AREF○  GND○←SD     A0○    A1○    A2○    A3○    A4○    A5○    y=3.81
BOT HDR   RST◉   3V◉    AREF◉    GND◉       A0◉    A1◉    A2◉    A3◉    A4◉    A5◉    y=1.27
```

---

## Wiring

### Wire Color Convention (Adafruit #1311)

| Color | Function |
|---|---|
| Red | 3V / VCC |
| Black | GND |
| Yellow | VOUT → A0 (analog signal) |
| Blue | MOSI |
| Green | MISO |
| White | SCK and CS (reused — physically separated) |

### Sensor Wires

| Wire | Color | From | To |
|---|---|---|---|
| w1 — pin 1 → A0 | Yellow | [1] hole (16.51, y=13.97) | A0○ dup pad (16.51, y=3.81) |
| w2 — pin 2 → GND | Black | [2] hole (19.05, y=13.97) | GND║ (13.97, y=13.97) |
| w3 — pin 3 → 3V | Red | [3] hole (21.59, y=13.97) | 3V║ (8.89, y=13.97) — passes through AREF hole, do not solder there |

### Capacitors (non-polarized — either leg in either hole)

| Cap | Value | Hole 1 | Hole 2 | Note |
|---|---|---|---|---|
| C1 | 470pF | [1] hole (16.51, y=13.97) | GND║ (13.97, y=13.97) | shares pin 1's hole — push both leads through together |
| C2 | 0.01µF | 3V║ (8.89, y=8.89) | GND║ (13.97, y=8.89) | AREF hole at 11.43 sits between legs, unused |
| C3 | 1µF | 3V║ (8.89, y=11.43) | GND║ (13.97, y=11.43) | AREF hole at 11.43 sits between legs, unused |

### MicroSD Wires

MicroSD breakout sits off-board. 6 bare wires soldered between breakout pads and Proto labeled pads:

| MicroSD breakout | Proto pad | x (mm) | y (mm) | Color |
|---|---|---|---|---|
| CLK | SCK inner | 31.75 | 3.81 | White |
| MOSI | MO inner | 34.29 | 3.81 | Blue |
| MISO | MI inner | 36.83 | 3.81 | Green |
| CS | pin 10 inner | 31.75 | 19.05 | White |
| VCC | 3V○ | 8.89 | 3.81 | Red |
| GND | GND○ | 13.97 | 3.81 | Black |

CS pin is GPIO 10 on the nRF52840 — same as most Feather boards.

---

## Assembly Order

**Philosophy: bench first, enclosure second.** Build and test everything on your desk before touching the enclosure.

### Stage 1 — Soldering (1–2 hours)

Work lowest-profile to tallest, and handle shared holes as a unit.

1. **Male headers** — first, no exceptions. Rest the board header-side down on a flat surface to keep them aligned while soldering.
2. **C2 and C3** — before the sensor goes in, while the bus strip holes at y=8.89 and y=11.43 are unobstructed.
3. **Sensor + C1 + wire to A0○ all at once** — these share the [1] hole. Pre-bend C1's legs to span from [1] to GND║. Thread all of the following before soldering anything:
   - Sensor pins 1–6 into the y=13.97 row (pin 1 marked end on the left)
   - C1 left leg and wire to A0○ both through the [1] hole alongside pin 1
   - C1 right leg through GND║ (13.97, y=13.97)
   - Wire far end through A0○ (16.51, y=3.81)
4. **Wires pin 2→GND║ and pin 3→3V║** — horizontal wires in the sensor row. w3 passes through the AREF hole at x=11.43 — thread through but do not solder there.
5. **Six SD card wires** — five to the bottom dup row, one to the top.

**What requires NO soldering:** stacking the boards, battery (JST connector), MicroSD card, panel mount USB cable.

### Stage 2 — Bench Testing (1–2 hours)

1. Stack the RTC FeatherWing onto the Feather and the FeatherWing Proto on top. Plug in the LiPo battery.
2. Connect the Feather to your computer via Micro-USB. Flash the firmware.
3. Open serial monitor. Verify analog values from the pressure sensor — squeeze the tube by hand and watch the numbers spike.
4. Tune the detection threshold: find the noise floor (no pressure), then find what a firm squeeze looks like.
5. Verify the two-pulse pairing logic catches a simulated bike (two quick squeezes) as one count.
6. Verify BLE is broadcasting and the display receives count increments.
7. Verify SD card logging — check that a file is being written with timestamps.

### Stage 3 — Enclosure Preparation

*Requires: weatherproof enclosure, PG-9 glands (#761), panel mount USB cable (#4056), step drill bit*

1. **Drill the PG-9 cable gland hole** — PG-9 glands use an M20 thread (20mm hole). Use a step drill bit. Place the hole on one of the short ends, near the bottom, so the tube exits horizontally close to the ground.
2. **Mount the panel mount USB cable** — the #4056 cable uses two M3 screws 20mm apart. Mount on the opposite short end from the tube.
3. **Thread the tube through the gland** before connecting it to the sensor.

### Stage 4 — Final Assembly

1. Mount the Feather stack in the enclosure using the M4 mounting bosses and standoffs.
2. Mount the FeatherWing Proto with the sensor nearby so the sensor barb aligns with the tube.
3. Press the tube onto the sensor barb — silicone stretches; it should grip firmly. Add a small cable tie if it feels loose.
4. Plug in the battery. Plug the panel mount cable into the Feather's Micro-USB port.
5. Seal around the panel mount cable with a small bead of silicone sealant.
6. Close the lid. Hand-tighten the screws evenly.

### Stage 5 — Deployment

1. Lay the tube across the bike lane. Keep it taut and flat to the ground.
2. Secure with duct tape or small ground staples every ~30cm.
3. Cap the far end — push a #8 machine screw (4.2mm) into the tube end and secure with a zip tie or hose clamp.
4. Mount the enclosure box to a post, wall, or ground stake near the edge of the lane.
5. Power on. Confirm the display increments when you walk a bike over the tube.

### Difficulty Summary

| Step | Difficulty | Notes |
|---|---|---|
| Soldering headers | Easy | Standard through-hole, beginner-friendly |
| Soldering MPX5010DP | Moderate | Bulky package, use a helping hand tool |
| Wiring FeatherWing Proto | Moderate | Keep wires short and tidy |
| Drilling enclosure | Moderate | Use a step drill bit, go slow |
| Threshold tuning | Moderate | Requires iteration — expect to reflash firmware once or twice |
| Tube deployment | Easy | Physical work, no electronics |

---

## Flashing Firmware

Requires `arduino-cli` (`brew install arduino-cli`). The Adafruit nRF52 core and SD library must be installed:

```bash
arduino-cli core install adafruit:nrf52
arduino-cli lib install "SD"
```

Connect the Feather via USB-C → Micro-USB. The port will appear as `/dev/cu.usbmodem101` (or similar).

**Compile:**
```bash
arduino-cli compile --fqbn adafruit:nrf52:feather52840 phase2/phase2.ino
```

**Upload:**
```bash
arduino-cli upload --fqbn adafruit:nrf52:feather52840 --port /dev/cu.usbmodem101 phase2/phase2.ino
```

**Monitor serial output:**
```bash
arduino-cli monitor --port /dev/cu.usbmodem101 --config baudrate=115200
```
