# Counter Noise-Storm Investigation Notes

## Confirmed: recurring false-trigger storms, time-of-day clustered (2026-08-02)

Analyzed a full day of `COUNTS.CSV` (2026-08-02, 4,034 logged events,
filtered into [`data/20260802.csv`](../data/20260802.csv)). Findings:

- **~62% of that day's "counts" (2,512 of 4,034 rows) are 34 distinct noise
  bursts** — runs of 10+ events firing less than 1 second apart. Real bike
  traffic can't produce that rate (a paired front/rear wheel needs
  `MIN_PAIR_GAP`–`MAX_PAIR_GAP` = 70–500ms just for the second pulse, let
  alone dozens of separate bikes per second).
- **Burst-event peaks cluster tightly at 66–72** (p90 = 72), just barely
  above the `THRESHOLD = 65` in `detection.h`. Real (non-burst) events have
  p10 = 83, median = 160. There's a clean gap between the two populations
  around peak ~75–80.
- **Bursts cluster by hour of day**: 10 in the 08:00 hour, 18 across
  14:00–16:00, almost none elsewhere. This is from a **single day**, so it's
  a pattern worth investigating, not an established cause — we have no
  logged temperature data, so "thermal" is one hypothesis among several that
  fit the same two windows (morning/afternoon vibration from nearby traffic,
  mowers, deliveries; an electrical/power cycle; something specific to that
  one day). It's the same failure mode already seen once before
  (a sealed enclosure produced an identical
  burst pattern in the 2026-08-01 test), but *why* it's still happening
  after the vent fix is unconfirmed.
- **No real bikes appear to be hiding inside the storms.** The few
  higher-peak spikes embedded mid-burst (e.g. a peak-272 event at 15:35:51)
  are sandwiched within ~1 second on both sides by the same 68–74 baseline
  noise — a real bike pulse pair needs a clean baseline gap around it per
  `MIN_PAIR_GAP`, not to be embedded mid-oscillation. These look like
  higher-amplitude excursions of the *same* noise process, not separate
  real detections. Practically: dropping whole burst windows should not
  cost any real bike counts.
- **Isolated peak=66/67 "single" events** also recur in ones and twos
  scattered randomly through the whole day (some over an hour from any
  burst). These sit right at/below the 68 "weakest real event" floor from
  the original 2026-05-31 characterization test, and are most likely
  baseline electrical/quantization noise ticking just over threshold, not
  real single-wheel traffic.
- **Working estimate for a genuinely trustworthy event: peak ≳ 80.** Not
  65 (current `THRESHOLD`), not even 68 (documented weakest real event) —
  the noise floor appears to have crept up into that range since the
  original characterization.

## Root-cause: unconfirmed

What's actually established: something is pushing the pressure sensor's
baseline output up close to `THRESHOLD` during these windows, and ordinary
ADC ripple around that elevated baseline crosses 65 repeatedly. The
firmware currently re-arms (`state = IDLE`) the instant the signal dips
below 65 (`detection.h`, `IDLE`/`BETWEEN` cases), so any oscillation right
around threshold gets counted as a rapid string of separate pulses — that
part of the mechanism is solid regardless of *why* the baseline drifts.

*Why* the baseline drifts during those specific windows is not established.
Thermal drift (solar heating, morning temperature ramp) is a plausible
hypothesis given the time-of-day clustering, but with no temperature
logging and only one day of data, it hasn't been distinguished from other
candidates (vibration from nearby activity at those hours, an electrical
noise source on a schedule, etc.). More days of `COUNTS.CSV`, or actual
temperature logging alongside it, would help confirm or rule this out
before investing in a thermal-specific mechanical fix (shading, etc.).

## Possible mitigations, roughly in order of effort

1. **Firmware — hysteresis (not yet implemented).** Use two levels: arm on
   `val > THRESHOLD` (65, unchanged) but only allow re-arming after `val`
   drops below a lower `REARM_THRESHOLD` (e.g. ~50, TBD from real data — see
   below). A signal wobbling between 60–72 never fully re-arms; a real bike
   pulse, which returns to baseline (~34), is unaffected. Directly targets
   the observed failure mode (oscillation pinned just above threshold) with
   no hardware changes.
2. **Firmware — raise `THRESHOLD`** to ~75–80 given the observed separation.
   Cheaper than hysteresis but blunter — risks losing the weakest real
   events (documented min real peak was 66–68).
3. **Firmware — storm/rate limiter.** Track crossing rate; suppress
   counting if crossings happen faster than physically possible for one
   bike (e.g. 10+ in under a second), independent of absolute peak value.
4. **Mechanical — re-check the pelican case vent.** It may not be
   equalizing pressure/temperature fast enough, if drift turns out to be
   thermal.
5. **Mechanical — shade the enclosure/tube from direct sun**, worth trying
   *if* the time-of-day pattern is confirmed thermal — premature otherwise.
6. **Mechanical — damping/secure tube mounting**, in case the pattern is
   vibration-driven instead (bursts fire roughly every 200–300ms, which
   doesn't by itself distinguish thermal drift from vibration).

Options 4-6 target a specific cause and shouldn't be pursued until the
time-of-day clustering is corroborated across more than one day (see below)
— right now they're guesses at *why*, not confirmed fixes.

## Next action: log the trough value, not just the peak

We can't currently tell *how far* the signal actually dips between storm
pulses — `COUNTS.CSV` only logs the peak of each already-completed pulse,
not the raw ADC waveform between them (the raw samples were never logged to
SD). That number is exactly what's needed to set `REARM_THRESHOLD` (option
1 above) with real data instead of a guess, and to distinguish it from a
real bike's between-wheel gap.

Recapturing this live via `sensor_debug.ino` over USB isn't practical — it
would mean tethering a laptop to the sensor while it sits outdoors, exposed
to real thermal conditions, during one of the storm-prone windows
(~08:00 or ~14:00–16:00), and hoping a storm occurs during that sitting.

Cheaper path: add a `troughSinceLast` field to `Detection` in `detection.h`
— track the minimum ADC value seen since the previous logged event, log it
alongside `peak` in `counts.csv`, and let it accumulate for free on every
future storm. Once we have that:

- Compare trough values during a storm vs. a real bike's between-wheel gap.
  If storms consistently don't drop as low as real gaps, hysteresis alone
  should work — set `REARM_THRESHOLD` in the gap between the two clusters.
  If storm troughs sometimes reach as low as real gaps, hysteresis won't
  fully solve it and the rate-limiter or a mechanical fix is the better bet.
- Replay real logged storms through both the current and hysteresis logic
  offline (e.g. a Python port of the state machine) using the *real*
  trough values, to confirm the fix actually collapses storm counts to
  near-zero without touching real bike counts, before flashing anything.
- After deploying, watch the next storm-prone window to confirm bike/single
  counts stay flat through it.

## Implemented (2026-08-04): trough, temperature, and periodic sampling

Ahead of a home data-collection test, three logging additions landed in
`phase2.ino`/`detection.h`, all designed to make raw SD volume actually
answer the drift question instead of just accumulating more
threshold-crossing events:

- **`trough`**: `Detection` now tracks the minimum ADC value seen since the
  previous logged event (updated every `tick()`, not just while a pulse is
  active) and includes it in `DetectionResult`. Reset to the current
  reading immediately after each `bike`/`single` emission.
- **`tempC`**: `RTC_DS3231::getTemperature()` (confirmed present in the
  installed RTClib, `RTClib.h:390`) is read on every logged row — directly
  tests the thermal-drift hypothesis that was previously unconfirmed for
  lack of any temperature data.
- **`sample` event type**: `loop()` now logs a `sample` row every
  `SAMPLE_INTERVAL_MS` (5000ms) regardless of whether the signal ever
  crosses `THRESHOLD` — `peak` holds the raw instantaneous ADC reading,
  `trough` is `-1` (not applicable to a point sample). This is the piece
  that actually lets baseline drift be observed even when nothing is being
  counted at all, which `bike`/`single`/`reset` rows alone can never show.

`counts.csv` header is now `datetime,event,peak,trough,tempC,count`
(previously `datetime,event,peak,count`) — old CSVs in `data/` are not
compatible with this schema and should not be concatenated with new ones
without re-tagging.

Compiled clean against `adafruit:nrf52:feather52840` (isolated build,
`sensor_debug.ino` excluded — see note below). Not yet flashed/uploaded.

**Next action:** flash and run the home test. Once storms are captured
with this schema, compare `trough` during storms vs. real bike
between-wheel gaps (per the analysis plan above), and check whether
`tempC` actually tracks the time-of-day clustering before treating thermal
drift as anything more than a hypothesis.

## Home test, session 1 (2026-08-04): storm confirmed as decay-tail of a
## mechanical disturbance, not an independent thermal/vibration event

**Sequence:** stomped/squeezed the tube hard (bike detections #1-5 in
`~/Desktop/COUNTS.CSV`). Baseline immediately spiked to ~900-926 (near
ADC max) and stayed pinned there for minutes — the detector logic just
sits parked in `IN_PULSE_1` while `val` never drops back under
`THRESHOLD`, which is why further stomping/squeezing produced zero
further counts; this wasn't a code bug, the sensor was saturated. Baseline
then declined slowly: 925 → 677 over the rest of that file (~15 min), the
board was power-cycled (to pull the SD card — `bikeCount` reset to 0
confirms this, but the tube itself was not touched during that cycle), and
in the next file (`~/Desktop/COUNTS_20260804_2.csv`) the same decline
continued: 677 → 390 at restart, continuing down through the 300s/200s/100s
over the next ~30 min, finally crossing the 65-90 storm band at 14:45:55
and firing a 65-event, 20-second storm (peaks 68-74, troughs 62-65) before
settling to a genuine quiet baseline of ~27-30 by ~14:50.

**Read on the storm-troughs-vs-real-gap question:** this storm's troughs
(62-65) sit clearly above the settled quiet baseline (~27-30), which is
also roughly where a real bike's between-wheel gap should land. That's a
real, usable gap for sizing hysteresis's `REARM_THRESHOLD` — e.g. ~45-50 —
though it's one data point, not a confirmed general separation.

**Reframing the "storm" investigation:** the original day-of-data
(2026-08-02, documented above) found storms clustering by hour with
"thermal" as one unconfirmed hypothesis among several. This session
suggests at least one class of storm is actually the *tail of a large,
slow-decaying mechanical disturbance* (something physically pressing on
or deforming the tube) passing back down through the threshold band on
its way to baseline — not necessarily a repeating daily
thermal/vibration pattern. Both mechanisms could coexist (a slow
disturbance-decay explains this session's storm; the original
time-of-day clustering across a full day without any known stomping event
is still unexplained by this). Don't conflate the two without more data —
but this is now positive evidence that at least one storm-producing
mechanism is a *decaying pressure excursion*, which argues for the
rate/trough-based mitigations already proposed (hysteresis, rate limiter)
regardless of the excursion's ultimate cause.

## Bench test with case closed (2026-08-04)

Closed the case (tube routed through the actual case gland) and squeezed
the tube by hand repeatedly while it logged to SD independent of USB
(`/Volumes/BIKECOUNTER/COUNTS.CSV`). Result: we were able to detect bikes
as expected, with the case closed — real, well-separated detections
(peaks 69-315) for the first ~35s. That rules out "tube/sensor/gland
broken" as the explanation for the earlier outdoor silence, and also
rules out closing the case by itself as the cause of that silence, since
detection worked fine with it closed. The pavement mounting/clamp point
remains the most likely untested variable for the outdoor silence.

After that initial ~35s, continued rapid hand-squeezing produced a run of
closely-spaced detections with peaks around 69-73 and troughs around
61-65 — numerically similar to the storm data logged earlier this session
(peaks 68-74, troughs 62-65, from the unrelated mechanical-decay episode).
Whether that similarity means anything about the trough/hysteresis
mitigation is genuinely unclear from this one session: it was uncontrolled
hand-squeezing, not real bike wheel-pairs, so it's not a reliable stand-in
for real traffic either way. Not drawing a conclusion from it — flagging
the numeric overlap as something to keep in mind, and leaving the
trough/hysteresis question open pending real outdoor multi-bike data.

## Push-down test, indoors, untaped (2026-08-04)

To separate "force/technique" from "taping" as candidates for the outdoor
silence, tried a gentler push-down (not a squeeze) on the untaped tube
indoors, watched live over serial. 6/6 pushes registered cleanly — peaks
96-118 (weaker than squeeze peaks, as expected for a lighter push),
troughs returning to baseline ~41-48 each time. So a gentler push, by
itself, still works fine when the tube is loose. That rules out
force/technique as the differentiator and leaves taping as the main
untested variable — next step is repeating this same push-down test with
the tube actually taped down, to see if the tape itself is what's
blocking the signal outdoors.

**Separate, more urgent finding — no signal at all from riding over
the tube:** in the last few minutes of `COUNTS_20260804_2.csv`
(15:04-15:08), multiple bike ride-overs were reported but the log shows
*zero* variation beyond normal ~2-4 ADC ripple around the settled 27-30
baseline — not even a sub-threshold blip. A working tube under a rolling
tire should produce some pressure signature even below `THRESHOLD`. This
points at a possible mechanical fault in the tube itself (kink, crimp, or
disconnection, plausibly from the earlier stomping) rather than anything
firmware-related, and needs a physical inspection of the tube before
further firmware tuning — no threshold/hysteresis change fixes a tube
that isn't transmitting pressure at all. Not yet inspected as of this
writing.

## Follow-up finding: unprompted oscillating decay, no contact (2026-08-04)

Re-examined the full `sample` trace in `COUNTS_20260804_2.csv` from
14:13-14:45 (leading up to the storm documented above). The user confirmed
nothing touched the tube/case during this window — it sat outside on the
driveway, observed but not approached. Despite that, the reading doesn't
decay smoothly: it oscillates up and down by tens of ADC counts (e.g.
380→434→381→300→361→254→336...) on top of the overall downward trend,
before finally crossing the storm band around 14:45. A passive process
(air leaking past a crimp, or silicone creeping back to shape) should look
like a smooth monotonic decay, not this repeated rise-and-fall — so
"decay tail" undersells what's happening; something was still actively
disturbing the signal throughout, not just settling.

Also notable: `tempC` (from the DS3231, physically adjacent to the
pressure sensor) rose smoothly and monotonically the entire time
(39.0°C → 44.5°C) while the ADC reading fell — the two are moving in
opposite directions, which argues against ambient thermal drift as the
driver of this particular event.

Candidate causes for the oscillation, unconfirmed, roughly ranked:

1. **Wind on the exposed reference port.** The sensor's reference port
   currently vents through an ~8" tube that exits the case via its own
   gland and is completely exposed/unshielded outdoors. A gust hitting
   that open tube end directly could produce exactly this kind of noisy,
   non-monotonic pressure signal, independent of anything happening in the
   main 15ft sensing tube.
2. **Stick-slip release of the stomped tube.** Silicone under a hard
   crimp doesn't necessarily relax smoothly — it can un-stick internally
   in discrete jumps, producing brief rises before resuming decay.
3. **Electronics warm-up drift.** This session started shortly after a
   board power cycle; ADC/voltage references can drift for the first
   ~10-30 min after power-on. Doesn't obviously explain oscillation lasting
   30+ min, but not ruled out.
4. **Ground/pavement vibration from nearby traffic**, or **hot asphalt
   subtly shifting how the tube is supported** — both possible without any
   direct contact, both hard to control for.

Long-term fix under consideration for (1): replace the reference tube's
cable gland with a PG-9 threaded submersible breather vent (membrane
lets air through slowly for pressure equalization, blocks fast gusts and
water) — confirmed the existing gland (Adafruit #761) is PG-9 thread, and
McMaster sells a PG-9 submersible threaded breather vent that should
thread into the same hole. Not yet purchased — the plan is to validate
the wind hypothesis for free first (see plan below) before spending on it.

## Testing plan for 2026-08-05

Goal: keep isolating the storm/oscillation cause without buying anything
yet. Also close out the taping question from the 08-04 push-down test.

1. **Set up outside** on the driveway as before, case closed, reference
   tube in its current (unshielded) configuration.
2. **Let it sit powered-on and untouched for ~45-60 min** before treating
   any of the following as a real observation window — this burns through
   any electronics warm-up drift so it doesn't confound the rest of the
   test.
3. **No-contact baseline window, unshielded** (~1 hour): reference tube
   exposed as-is, nobody near it, log samples only. Note wind/weather
   qualitatively a few times during the hour (not just once at the start),
   since conditions can drift within the window itself.
4. **No-contact baseline window, shielded** (~1 hour), run right after (3):
   loosely pack a wad of cotton/foam a short way into the reference tube
   opening (breathable, reversible, free) to break up direct gusts without
   sealing the tube. Compare the `sample` trace against (3) — persistent
   oscillation unshielded but flat shielded implicates wind; no difference
   points back to warm-up drift or stick-slip release instead. Caveat:
   stacking two 1-hour windows means ~2 hours elapse between the start of
   (3) and the end of (4), long enough for real weather to drift on its
   own — treat a difference between windows as suggestive, not conclusive,
   unless the noted wind/weather conditions actually line up with it.
5. **Tape the main sensing tube down** as it would be for a real
   deployment.
6. **Manually push down on the taped tube** (same gentle push, not squeeze,
   as the 08-04 indoor test) and confirm on live serial whether it still
   registers cleanly. This is the direct test of whether taping itself is
   what killed detection outdoors on 08-04.

Deferred until steps above give a clear signal: buying the PG-9 submersible
vent (only if step 4 shows shielding clearly helps — no point spending $20
validating a mechanism that free cotton either already confirmed or ruled
out).

Still open, not addressed by tomorrow's plan: whether the trough/hysteresis
mitigation actually separates storms from real bike gaps (needs real
outdoor multi-bike data, not bench/hand tests); the original
2026-08-02 time-of-day-clustered storms (no known stomping event, separate
from this session's mechanical-decay storm); the BLE-receiving-packets gap
for the display-power investigation.

## Field session notes (2026-08-05): live pressure-on-display debugging

Watching the raw A0 reading on the display in real time (rather than only
reviewing logged CSVs after the fact) turned out to be the single most
effective debugging method used so far — problems that are invisible in
a post-hoc peak/trough log show up immediately as a live number.

- **Tube configuration finding:** a short, uncapped tube on the reference
  port is the best configuration seen so far. A capped tube on the
  reference port had previously given systematically low, unresponsive
  readings. Bikes should still ride over a long capped tube on the
  *sensing* port, as before — only the reference-port tube changed.
- **Baseline pressure fluctuates on its own** — slow rises and falls with
  no contact, consistent with the wind/thermal/warm-up drift already
  suspected in the sections above, now directly *seen* happening live
  rather than inferred after the fact from logged storms.
- **Reseating the hose on the sensor barb changed the reading** at one
  point during this session — disconnecting and reattaching the tube-to
  sensor connection visibly shifted the baseline. This is a new candidate
  noise source (a loose/marginal barb connection) that hasn't been
  isolated from the other drift causes yet — worth deliberately wiggling/
  reseating the barb connection in a controlled way in a future session to
  see if it's reproducible.
- **Working hypothesis going forward:** given that baseline drifts on the
  order of a minute or so, a fixed `THRESHOLD` may not be viable long-term
  — the firmware likely needs to track a rolling/adaptive baseline
  (recomputed every ~minute or so) and detect spikes *relative to that
  baseline* rather than against a fixed ADC value. Not yet designed or
  implemented — flagged here as the emerging direction suggested by today's
  live-viewing session, to weigh against the trough/hysteresis mitigation
  already discussed above.

## System volatility review (2026-08-09): rebuild vs. power reconfiguration

The adaptive-baseline detector (`OFFSET`-above-`baseline` in `detection.h`,
the direction flagged in the section above) is now implemented and
tuned down to `OFFSET = 20` / `MIN_PULSE_MS = 8` after several rounds of
missed-bike and noise-storm tradeoffs. In the same session, several
unrelated-looking symptoms showed up close together:

- SD logging (`counts.csv`) failed completely for one session — 0 bytes,
  not even the header row `setup()` always writes — with no code change in
  the SD path to explain it. Most likely a rapid flash/reset cycle leaving
  the card or SPI bus in a bad transient state, not a firmware regression.
- A power-on analog transient: initial raw ADC readings in the 300s on the
  first two of three rapid power cycles (within ~30s), back to a normal
  ~40 on the third. Resolves with a longer power-off interval — consistent
  with residual charge on decoupling/filter capacitors not fully bleeding
  off between quick cycles, though not confirmed against the alternative
  of internal sensor settling behavior.
- Detection sensitivity has needed three rounds of threshold/timing
  tuning (`OFFSET` 40→20, `MIN_PULSE_MS` 2→8) to reliably catch real bikes
  without reopening the noise-storm problem documented above.

None of these were traced to a single root cause. But several of them —
the startup transient and the SD write failure in particular — are the
kind of symptom a marginal/noisy power rail can produce simultaneously
(both BLE radio bursts and SD writes are current spikes that can sag a
weak supply, which can also disturb whatever the ADC is referencing).
That's a hypothesis, not a diagnosis: no measurements have been taken to
confirm it.

### Does a rebuild make sense right now?

Not yet. Nothing observed so far points at a specific failed/degrading
part — the pattern is broad, low-grade flakiness across subsystems
(analog, SD, BLE) rather than one component behaving consistently wrong.
Rebuilding (~$84-105 reusing the existing SD card and battery — see
`README.md` BOM) would mean guessing which part to replace without
evidence it's the culprit, on hardware that mostly works: real bikes are
being detected correctly at current settings once the startup window
passes. Worth revisiting if a specific part starts showing consistent,
reproducible failure (e.g. the startup transient recurs even after a full
clean power-down, or SD writes fail more than once).

### Power reconfiguration: a smaller purchase to validate before a rebuild, not a free alternative to one

The MPX5010DP is currently run under-spec at 3.3V (community-proven
workaround, per `phase1/README.md`) instead of its rated 5V, which
compresses its output span to ~0.13-3.07V instead of the full ~0.2-4.7V.
A smaller absolute signal window for the same real-world pressure change
means worse signal-to-noise for a fixed amount of ADC/electrical noise —
plausibly a contributing factor to how much threshold-tuning effort this
project has needed across both the storm investigation above and today's
sensitivity tuning.

This is not a way to avoid spending money instead of rebuilding — it
still requires buying parts, just fewer/cheaper ones, and it answers a
different question than a rebuild does. A full rebuild
(~$84-105, see above) replaces parts on a guess. This instead tests, on a
separate breadboard rig (Phase-1-style — a spare MPX5010DP ~$25, a small
5V boost converter breakout ~$5-10, two resistors, a multimeter; no
soldering, and the deployed field unit is never touched), whether
powering the sensor at its rated 5V with a resistor divider scaling
`VOUT` back under the nRF52840's ADC-safe ~3.6V limit produces a cleaner
signal than the current 3.3V under-volt. If it does, that's evidence
worth acting on (redesign the sensor's power path); if it doesn't, that
rules out supply voltage as a factor without having spent on a full
rebuild first. Either way, money gets spent before an answer exists —
this just spends less of it to get a narrower answer.

Not yet attempted as of this writing. Not yet decided whether it's worth
doing at all, given the added cost and effort on top of everything else
this project has already required.
