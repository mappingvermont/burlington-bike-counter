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

- [x] Build display enclosure (PVC, PVC cement)
- [x] Test display enclosure outside
- [x] Investigate options to hide sensor box and lock it
- [ ] Order sensor box, waterproof power connector, cable locks
- [ ] Drill hole in enclosure to allow power connection
- [ ] Drill hole in sensor box to allow tube through

---

## Stage 4 — Hardening

- [ ] General refactor / simplification of code
- [ ] Improve energy management for both sensor and display
- [ ] Update display to turn off between 10 PM and 6 AM
- [ ] Screw in display side panel using tamper resistant screws
- [ ] Seal everything with silicone sealant
- [ ] Silica packets everywhere

---

## Stage 5 — Deployment

- [ ] Mount and lock display enclosure on fence
- [ ] Install sensor enclosure + lock
- [ ] Power on — confirm display increments when bike rolls over tube

