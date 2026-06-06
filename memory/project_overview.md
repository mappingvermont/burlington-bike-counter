---
name: project-overview
description: Burlington bike counter — current build status and next steps
metadata:
  type: project
---

Stable architecture and key files are documented in CLAUDE.md.

**Current status (as of 2026-06-06):** Phase 2 bench testing complete. RTC timestamps integrated and verified. SD logging working (appends across sessions). BLE advertising verified. Detection logic working with squeeze testing.

**Why:** Hardware stack now includes DS3231 RTC FeatherWing between Feather and Proto board — stacking header seating is sensitive, reseat if A0 reads ~320 instead of ~34.

**Next:** Stage 3 — enclosure preparation (drill 20mm PG-9 gland hole, panel mount USB-C cutout, then final assembly).
