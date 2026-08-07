#include <catch2/catch_test_macros.hpp>
#include "detection.h"

// Advance time from `from` to `to` (inclusive) at a fixed ADC value.
// Returns the last non-NONE event seen, or {DE_NONE, 0, 0}.
static DetectionResult advance(Detection& d, unsigned long from, unsigned long to, int adc) {
    DetectionResult last = {DE_NONE, 0, 0};
    for (unsigned long t = from; t <= to; t++) {
        auto r = d.tick(t, adc);
        if (r.type != DE_NONE) last = r;
    }
    return last;
}

// Detection now tracks an adaptive baseline (baseline + OFFSET) instead of a
// fixed THRESHOLD, and fast-smooths the raw ADC value (FAST_ALPHA) before
// comparing it to that threshold. baseline/smoothed both initialize from the
// very first sample, so priming with a run of constant "quiet" samples pins
// the baseline at that level. All tests start pulses at t=200 to clear the
// initial COOLDOWN window (lastCount initializes to 0; COOLDOWN_MS=200).
//
// FAST_ALPHA is high enough (0.9) that smoothed settles to >90% of a step
// change within a single sample, so pulse-boundary timing in these tests
// matches what the raw signal would do. Exact smoothed peak values still lag
// the raw peak slightly, so peak-value assertions use rawPeak, which tracks
// the unsmoothed signal exactly.

TEST_CASE("noise below threshold produces no event") {
    Detection d;
    for (unsigned long t = 0; t < 1000; t++) {
        auto r = d.tick(t, 50);  // constant "quiet" level; baseline locks to it
        REQUIRE(r.type == DE_NONE);
    }
    REQUIRE(d.bikeCount == 0);
    REQUIRE(d.state == IDLE);
}

TEST_CASE("pulse shorter than MIN_PULSE_MS is rejected") {
    Detection d;
    // 1ms pulse — below MIN_PULSE_MS (2ms)
    advance(d, 0, 199, 0);
    d.tick(200, 100);  // enter IN_PULSE_1 at t=200
    auto r = d.tick(201, 0);  // drop at t=201 — duration = 1ms < MIN_PULSE_MS
    REQUIRE(r.type == DE_NONE);
    REQUIRE(d.state == IDLE);
    REQUIRE(d.bikeCount == 0);
}

TEST_CASE("single pulse counts as single after MAX_PAIR_GAP") {
    Detection d;
    // Front wheel: t=200-210
    advance(d, 0, 199, 0);
    advance(d, 200, 210, 100);
    // Drop below: pulse1End = 211
    d.tick(211, 0);
    // No rear wheel — wait past MAX_PAIR_GAP (timeout fires at 211 + MAX_PAIR_GAP + 1)
    auto r = advance(d, 212, 212 + MAX_PAIR_GAP, 0);
    REQUIRE(r.type == DE_UNPAIRED);
    REQUIRE(d.bikeCount == 1);
}

TEST_CASE("normal bike pass fires DE_PAIRED") {
    Detection d;
    // Front wheel: t=200-210, pulse1End=211
    advance(d, 0, 199, 0);
    advance(d, 200, 210, 100);
    d.tick(211, 0);
    // Gap until MIN_PAIR_GAP: rear wheel can start at 211+70=281
    advance(d, 212, 280, 0);
    // Rear wheel: t=281-291
    advance(d, 281, 291, 100);
    auto r = d.tick(292, 0);
    REQUIRE(r.type == DE_PAIRED);
    REQUIRE(d.bikeCount == 1);
    REQUIRE(d.state == IDLE);
}

TEST_CASE("bounce pulse before MIN_PAIR_GAP is ignored") {
    Detection d;
    // Front wheel: t=200-210, pulse1End=211
    advance(d, 0, 199, 0);
    advance(d, 200, 210, 100);
    d.tick(211, 0);
    // Bounce at t=220: gap = 220-211 = 9ms < MIN_PAIR_GAP (70ms) — must be ignored
    advance(d, 212, 219, 0);
    advance(d, 220, 230, 100);  // bounce pulse
    advance(d, 231, 250, 0);
    REQUIRE(d.state == BETWEEN);
    REQUIRE(d.bikeCount == 0);
}

TEST_CASE("valid rear wheel after bounce still counts") {
    Detection d;
    // Front wheel: pulse1End=211
    advance(d, 0, 199, 0);
    advance(d, 200, 210, 100);
    d.tick(211, 0);
    // Bounce at t=220 (ignored)
    advance(d, 220, 225, 100);
    advance(d, 226, 280, 0);
    // Valid rear wheel at t=281 (281-211=70 >= MIN_PAIR_GAP)
    advance(d, 281, 291, 100);
    auto r = d.tick(292, 0);
    REQUIRE(r.type == DE_PAIRED);
    REQUIRE(d.bikeCount == 1);
}

TEST_CASE("cooldown blocks pulse immediately after count") {
    Detection d;
    // First bike
    advance(d, 0, 199, 0);
    advance(d, 200, 210, 100);
    d.tick(211, 0);
    advance(d, 212, 280, 0);
    advance(d, 281, 291, 100);
    d.tick(292, 0);  // DE_PAIRED fires, lastCount=292
    REQUIRE(d.bikeCount == 1);
    // New pulse immediately — within COOLDOWN_MS (200ms)
    advance(d, 293, 400, 100);
    d.tick(401, 0);
    REQUIRE(d.bikeCount == 1);  // not double-counted
}

TEST_CASE("peak value is tracked correctly across pulse") {
    Detection d;
    advance(d, 0, 199, 0);
    d.tick(200, 80);
    d.tick(201, 90);
    d.tick(202, 120);  // raw peak
    d.tick(203, 85);
    d.tick(204, 0);    // pulse1End=204
    auto r = advance(d, 205, 205 + MAX_PAIR_GAP, 0);
    REQUIRE(r.type == DE_UNPAIRED);
    // rawPeak tracks the unsmoothed signal exactly; peak (smoothed) lags it.
    REQUIRE(r.rawPeak == 120);
    REQUIRE(r.peak > OFFSET);
    REQUIRE(r.peak <= r.rawPeak);
}
