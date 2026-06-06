# Road Tube Bike Counter — Remaining Steps

## Status
- [x] Phase 1 BOM acquired
- [x] FeatherWing Proto soldered (sensor, capacitors, SD wires)
- [x] Firmware flashed
- [x] Sensor verified (37 at rest, ~560 at squeeze)

---

## Stage 2 — Bench Testing (in progress)

- [x] Roll a bike tire over the tube outdoors — verify two spikes ~0.5–1s apart
- [x] Determine threshold value — 65 (noise floor 34, weakest real event peak 68)
- [x] Write detection firmware (threshold logic + two-pulse pairing → one bike count)
- [x] Verify BLE is broadcasting and display receives count increments
- [x] Insert SD card and verify logging
- [x] Solder DS3231 stacking headers and re-integrate RTC timestamps
- [x] Update code to log timestamps

---

## Stage 3 — Enclosure Preparation

*Requires: enclosure (#905), PG-9 glands (#761), panel mount USB cable (#4056), step drill bit*

- [ ] Drill 20mm hole on one short end (near bottom) for PG-9 tube gland
- [ ] Drill mounting holes + cutout on opposite short end for panel mount USB-C socket
- [ ] Thread tube through gland before connecting to sensor

---

## Stage 4 — Final Assembly

- [ ] Mount Feather stack in enclosure using M4 standoffs
- [ ] Align sensor barb with tube entry
- [ ] Press tube onto sensor barb (add cable tie if fit feels loose)
- [ ] Plug in LiPo battery (JST-PH)
- [ ] Connect panel mount cable to Feather Micro-USB port
- [ ] Seal around panel mount cable with silicone sealant
- [ ] Close and hand-tighten lid evenly

---

## Stage 5 — Deployment

- [ ] Lay tube across bike lane, taut and flat
- [ ] Secure every ~30cm with duct tape or ground staples
- [ ] Cap far end (4.2mm machine screw + zip tie)
- [ ] Mount enclosure to post or ground stake at lane edge
- [ ] Power on — confirm display increments when bike rolls over tube

---

## Open Questions

- How to anchor the sensor box so it isn't displaced by traffic or weather
- Display mounting location and orientation
