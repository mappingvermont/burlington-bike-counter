# Display Power/Runtime Investigation Notes

## Confirmed: panel-dark, board-alive fault (2026-08-02)

During install, the display panel was lit for the first ~30 min, then found dark
~2 hrs later. On inspection:

- The board's status NeoPixel was flashing green — CircuitPython's documented
  "code.py running with no error" signal — so the ESP32-S3 never lost power or
  crashed.
- Power screw terminals and the HUB75 ribbon into the panel's left JIN connector
  were both fully seated.
- A power cycle (unplug/replug) brought the panel back immediately.

**Mechanism:** a stale `rgbmatrix.RGBMatrix()` handshake — the panel stopped
responding at some point after the first 30 min (working fine), and since
`RGBMatrix()` doesn't retry its handshake once initialized, it stayed dark until a
fresh boot re-initialized it.

**Why the handshake broke is unknown**, and connectors being found fully seated
afterward doesn't rule out a physical cause — nobody checked their state *before*
handling the unit. But research into how this board actually works surfaces a much
more concrete lead than "loose wire": a documented class of ESP32-S3-specific
resource contention between the display driver and other on-chip activity.

**Research findings (2026-08-02):**

- The Matrix Portal S3's HUB75 refresh is driven by Protomatter as a separate,
  DMA/interrupt-driven peripheral task in C — it does **not** run as part of the
  main CircuitPython loop. The NeoPixel's green flash only confirms the *Python*
  loop (our BLE scan) is running without error; it says nothing about whether the
  display refresh task is still alive. This alone explains how the board can look
  completely healthy while the panel is dark: they're two independent subsystems,
  and only one was being observed.
- Adafruit's own `Adafruit_Protomatter` library has a confirmed, reproducible bug
  on ESP32-S3 (not seen on the older M4-based Matrix Portal) where filesystem
  writes cause display corruption — a real instance of flash-bus contention
  between the display driver and another on-chip operation
  ([Adafruit_Protomatter#71](https://github.com/adafruit/Adafruit_Protomatter/issues/71)).
  Community ESP32 HUB75 projects report a broader version of the same class of
  issue between I2S/DMA-driven panel refresh and WiFi.
- Our `code.py` runs `_bleio.adapter.start_scan()` back-to-back with **no delay**
  between scan windows (confirmed by reading the file — the BLE duty-cycle
  described in earlier design discussions was never actually implemented).
  Continuous radio activity with no idle time is exactly the kind of sustained
  load implicated in these contention bugs.
- BLE support on ESP32-S3 CircuitPython is newer and less mature than WiFi, with
  its own documented hard-fault bugs (e.g.
  [circuitpython#9708](https://github.com/adafruit/circuitpython/issues/9708)) —
  though that specific bug involves peripheral-mode pairing, not the passive
  central-mode scanning this code does, so it's a data point on radio-stack
  maturity rather than a direct match.

**Working hypothesis:** sustained BLE scanning (and/or background WiFi from the
Web Workflow supervisor) is intermittently starving or corrupting the display's
DMA-driven refresh task, which then requires a full reboot to re-establish —
while the foreground Python loop is completely unaffected and keeps reporting a
clean "no error" heartbeat throughout. This is not confirmed, but it's a
mechanism with real precedent on this exact chip, unlike "the connector jiggled
loose."

**Possible mitigations worth trying, roughly in order of effort:**
1. Add a short `time.sleep()` between BLE scan windows in `code.py`, so the radio
   isn't keeping the chip continuously busy — reduces the sustained-load condition
   implicated above, and was already flagged elsewhere as a good idea for other
   reasons.
2. Disable the Web Workflow's WiFi station mode for field deployment (only enable
   it for at-home maintenance sessions), removing one of the two radio sources
   competing with the display refresh.
3. If the problem persists, it may be worth prototyping against
   `ESP32-HUB75-MatrixPanel-DMA` (a community C++/Arduino library with more
   granular DMA/clock controls) to see if the issue is CircuitPython/Protomatter-
   specific — though that would mean leaving CircuitPython for this board.

**Rule of thumb going forward:** if the panel goes dark, check the status NeoPixel
first.
- Flashing green → board is fine, this is the handshake fault → power cycle fixes it.
- Anything else (off, red flash pattern, solid non-green) → different problem,
  investigate separately (CircuitPython flashes red on an unhandled exception).

## Next actions (2026-08-02)

To isolate whether WiFi/BLE radio contention is actually the cause, reverted both
suspect variables at once and are now running a battery test to see if the panel
still goes dark:

- **`code.py`**: reverted to `bit_depth=1` (no PWM dimming) and the original amber
  `0xFF3800` color — the dimmed/green settings were too dim anyway, independent of
  this investigation.
- **`settings.toml`**: WiFi/Web Workflow credentials commented out (not deleted —
  easy to restore for at-home maintenance). Removes background WiFi as one of the
  two suspected radio-contention sources. Reasonable to disable for field
  deployment since the enclosure is open for physical access anyway.
- BLE scanning (`_bleio.adapter.start_scan()`, no delay between windows) is
  **unchanged** — still a suspected contention source, but not touched in this
  round so it isn't a confound in the test.

**Test in progress:** running on battery to see whether the panel still goes dark.
If it doesn't, that points at WiFi/Web Workflow (now disabled) as the cause. If it
still goes dark, BLE scanning or something else is implicated instead, and the
`time.sleep()`-between-scans mitigation is the next thing to try.
