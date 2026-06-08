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

- [ ] Build display enclosure (PVC, PVC cement)
- [ ] Figure out ESP32 and battery mounts
- [ ] Test display enclosure outside
- [ ] Investigate options to hide sensor box and lock it
- [ ] Order sensor box
- [ ] Drill hole in sensor box to allow tube through

---

## Stage 4 — Hardening

- [ ] Add eye bolts and lock + hasp to display
- [ ] Screw in display side panel using tamper resistant screws
- [ ] Seal everything with silicone sealant
- [ ] Silica packets everywhere

---

## Stage 5 — Deployment

- [ ] Lay tube across bike lane, taut and flat
- [ ] Mount and lock display enclosure on fence
- [ ] Install sensor enclosure + lock
- [ ] Power on — confirm display increments when bike rolls over tube

