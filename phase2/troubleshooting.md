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

This firmware change (`troughSinceLast` field + CSV column) has been
scoped but **not yet implemented** — next session, do this before
attempting the hysteresis fix itself.
