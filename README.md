# Burlington Bike Counter

A DIY pneumatic road tube bike counter. Built using a sealed silicone air tube laid across a bike lane to detect pressure spikes from passing tires. An electronic sensor counts them, timestamps each event, and broadcasts the running total over BLE to a nearby LED display.

## System Architecture

```
[Silicone tube across bike lane]
        ↓ pressure pulse
[Weatherproof box, roadside]
  MPX5010DP sensor + Feather nRF52840 + DS3231 RTC FeatherWing + microSD breakout + LiPo
        ↓ BLE
[Matrix Portal ESP32-S3]
  plugged into 64×32 RGB panel
  powered by USB power bank
```

## Build Phases

- **[Phase 1](phase1/)** — Proof of concept. Sensor on a breadboard, laptop power, serial monitor. ~$85. Proves the MPX5010DP detects a tire before any deployment hardware is ordered.
- **[Phase 2](phase2/)** — Full deployment. RTC, SD logging, LiPo battery, weatherproof enclosure. ~$83.
- **[Display](display/)** — Matrix Portal S3 + 64×32 RGB panel. Receives BLE from the sensor unit and renders the count as amber digits.

## Status

See [next-steps.md](next-steps.md) for current progress and remaining tasks.

## Photos

**Bench testing**

<img width="2016" height="1512" alt="Image" src="https://github.com/user-attachments/assets/afc454f4-54b0-4a09-8e8a-92fd8513c2af" />

**Sensor board**

<img width="2016" height="1512" alt="Image" src="https://github.com/user-attachments/assets/1ae3ad1a-bfe2-4e69-b90e-cf1811a93718" />

**Display**

<img width="1512" height="2016" alt="Image" src="https://github.com/user-attachments/assets/b8d8d387-3835-4abe-a23d-e579d8f8a5a6" />

## Costs

| Section | Total |
|---|---|
| [Phase 1](phase1/) | ~$85 |
| [Phase 2](phase2/) | ~$83 |
| [Display electronics](display/electronics.md) | ~$135 |
| [Display enclosure](display/enclosure.md) | ~$106* |
| **Total** | **~$409** |

\* Enclosure total excludes a handful of unpriced hardware-store/Amazon items (caulk, eyebolts, padlock, waterproof USB-C mount, desiccant) — see [enclosure.md](display/enclosure.md) for the full BOM.

---

## Prior Art

The pneumatic tube counter is a well-established technology used by highway departments worldwide since the 1960s. Commercial systems (MetroCount, JAMAR, Diamond Traffic) use the same sensing principle — pneumatic tube, pressure transducer, pulse detection — just hardened for production.

All prior DIY art uses Arduino Pro Mini or Uno with 5V sensors. This build uses nRF52840 + BLE with the same MPX5010DP sensor, running at 3.3V.

### DIY Reference Builds

- **[jekhor/pneumatic-tc-firmware](https://github.com/jekhor/pneumatic-tc-firmware)** — only project explicitly built as a bicycle counter. Has low-pass filtering, SD logging, command interface. Most architecturally complete.
- **[SarahDal/ArduinoCC](https://github.com/SarahDal/ArduinoCC)** — best-documented build. Field-tested, three code variants, real battery life data (~147hrs on 1900mAh). Uses MPX5100DP.
- **[neuroprod/Road-Tube-Traffic-Counter](https://github.com/neuroprod/Road-Tube-Traffic-Counter)** — the Hackaday reference design. Minimal but working. Uses MPX5010DP — same sensor as this build. [Hackaday page](https://hackaday.io/project/4567-traffic-counter-road-tube), [YouTube demo](https://www.youtube.com/watch?v=x1HM3IvExJE).
- **[civictechdc/traffic_counter](https://github.com/civictechdc/traffic_counter)** — original 2012 open-source project by Tomorrow Lab NYC, forked and maintained by Code for DC.

