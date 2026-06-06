# Phase 1 — Proof of Concept

Prove the sensor works before ordering deployment hardware. Power from laptop via Micro-USB; view sensor readings in serial monitor. The only soldering is the Short Headers Kit onto the Feather (easiest possible solder job).

**What you're proving:**
- The MPX5010DP detects a tire rolling over the tube
- What the noise floor vs. signal looks like in your specific setup
- What threshold values work for a bike
- That the two-pulse pairing timing is correct

**What you're NOT testing yet:** BLE, SD logging, battery life, weatherproofing.

The Feather won't sit in the breadboard (female headers don't go into a breadboard). Sensor goes in the breadboard, Feather sits beside it connected by 3 jumper wires. Tape both to a piece of cardboard to make one stable outdoor-ready unit.

---

## BOM (~$101)

### Adafruit
| # | Part | Product | Price |
|---|---|---|---|
| 1 | Feather nRF52840 Express | [#4062](https://www.adafruit.com/product/4062) | $24.95 |
| 2 | Short Headers Kit for Feather | [#2940](https://www.adafruit.com/product/2940) | $1.50 |
| 3 | Half-size breadboard | [#64](https://www.adafruit.com/product/64) | $5.00 |
| 4 | Jumper wires (male-to-male) | [#758](https://www.adafruit.com/product/758) | $3.95 |
| 5 | Brass tip cleaner | [#1172](https://www.adafruit.com/product/1172) | $5.00 |
| 6 | Helping hands | [#291](https://www.adafruit.com/product/291) | $6.95 |
| 7 | Solder wick, 1.5mm wide | [#149](https://www.adafruit.com/product/149) | $3.50 |

### Mouser
| # | Part | Link | Price |
|---|---|---|---|
| 8 | MPX5010DP pressure sensor | [Mouser](https://www.mouser.com/ProductDetail/NXP-Semiconductors/MPX5010DP) | $21.48 |

Datasheet: [MPX5010.pdf](MPX5010.pdf)

### Amazon
| # | Part | Link | Price |
|---|---|---|---|
| 9 | 4mm ID silicone tubing, black, 5m | [uxcell 4mm ID × 6mm OD, black, 16.4ft](https://www.amazon.com/uxcell-Silicone-Irrigation-Brewing-Aquaponics/dp/B0FCFQWM82?th=1) | ~$10 |
| 10 | Ceramic capacitor assortment (1µF, 0.01µF, 470pF needed) | [Molence 1200pcs 24-value through-hole kit](https://www.amazon.com/Molence-Multilayer-Monolithic-Electronics-Audio-Video/dp/B09WRPHNK8) | ~$18 |

### Hardware Store
| # | Part | Notes | Price |
|---|---|---|---|
| 11 | Hose end plug/cap (4mm) | Push a #8 machine screw (4.2mm) into the tube end, secure with a zip tie or hose clamp | Pennies |

---

## Sensor Notes

### MPX5010DP

Rated 5V but community-proven to work at 3.3V. Output at 3.3V supply: 0.13–3.07V, safely within the nRF52840 ADC range.

**P1 vs P2:** Per datasheet Table 7, P1 is the side with the part marking ("MPX5010DP" printed on the body). Blow into P1 to increase VOUT; blowing into P2 decreases VOUT.

**Pin mapping:**
```
 ┌──────────────────────┐
 │   MPX5010DP          │  ← part marking = pin 1 end
 └──────────────────────┘
  │  │  │  │  │  │
  1  2  3  4  5  6
VOUT GND VS  NC  NC  NC
```

### Capacitor Placement (breadboard)

Datasheet Figure 11 lists these as **required to meet spec** — add before concluding the sensor is unresponsive.

Sensor in breadboard: pin 1 (VOUT) at row 5, pin 2 (GND) at row 6, pin 3 (VS) at row 7.

```
row 5 (VOUT) ── C1 (471)  ── row 6 (GND)
row 7 (VS)   ── C2 (103)  ── row 6 (GND)
row 7 (VS)   ── C3 (105)  ── row 6 (GND)  ← different column from C2
```

Ceramic caps are non-polarized — either lead in either row. Use columns b–e (any column right of the sensor pins).

---

## Wiring (breadboard → Feather)

| Sensor pin | Function | Wire color | Feather pin |
|---|---|---|---|
| 1 | VOUT | Yellow | A0 |
| 2 | GND | Black | GND |
| 3 | VS | Red | 3V |
| 4–6 | n.c. | — | — |

Double-tap reset button to enter bootloader before flashing.

---

## Arduino Sketch

```cpp
#include <Adafruit_TinyUSB.h>

void setup() { Serial.begin(115200); }

void loop() {
  int val = analogRead(A0);
  int bars = map(val, 80, 300, 0, 40);
  bars = constrain(bars, 0, 40);
  Serial.print(val);
  Serial.print(" |");
  for (int i = 0; i < bars; i++) Serial.print("=");
  Serial.println();
  delay(50);
}
```

Noise floor at rest: ~34. Pressure spike should stretch the bar visibly.

Observed values after outdoor testing: noise floor 34, weakest real event peak 68, detection threshold set to 65.

---

## Soldering Tools

### Essential

**Soldering iron** — a temperature-controlled iron makes a significant difference. Two options:
- **Pinecil** (~$30) — USB-C powered, excellent for occasional use
- **Hakko FX-888D** (~$100) — standard workhorse

Set to ~350°C for leaded solder, ~370°C for lead-free.

**Solder** — 60/40 or 63/37 rosin-core, 0.8mm diameter. Leaded (60/40) is easier for beginners.

**Flush cutters** — to trim header pins after soldering. ~$5–10.

**Multimeter** — for testing connections and voltage. AstroAI AM33D or any ~$15–20 model is fine.

### Very Useful

**Helping hands / PCB holder** — particularly useful for the MPX5010DP, which is bulky. ~$10.

**Solder wick** — copper braid for absorbing solder when fixing bridged joints. ~$5.

### Ventilation

Work near an open window or point a small fan to blow fumes away. A fume extractor (~$20–30) is better but not essential for occasional use.
