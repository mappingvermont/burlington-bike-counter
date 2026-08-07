#pragma once
#include <stdint.h>

const float OFFSET      = 40.0f;
const float TAU_MS      = 10000.0f;
const float FAST_ALPHA  = 0.9f;   // per-sample smoothing to reject single-sample ADC noise
const int MIN_PULSE_MS = 2;
const int MIN_PAIR_GAP = 70;
const int MAX_PAIR_GAP = 500;
const int COOLDOWN_MS  = 200;

enum DetectionEvent { DE_NONE, DE_UNPAIRED, DE_PAIRED };
struct DetectionResult { DetectionEvent type; int peak; int rawPeak; };
enum State { IDLE, IN_PULSE_1, BETWEEN, IN_PULSE_2 };

struct Detection {
    State state              = IDLE;
    unsigned long pulseStart = 0;
    unsigned long pulse1End  = 0;
    unsigned long lastCount  = 0;
    int pulsePeak            = 0;
    int rawPeak              = 0;
    int bikeCount            = 0;
    float baseline           = 0.0f;
    float smoothed           = 0.0f;
    unsigned long lastBaselineUpdate = 0;
    bool baselineInit        = false;
    bool smoothedInit        = false;

    DetectionResult tick(unsigned long now, int val) {
        if (!smoothedInit) {
            smoothed = val;
            smoothedInit = true;
        } else {
            smoothed += FAST_ALPHA * (val - smoothed);
        }

        // Baseline tracks the (fast-smoothed) signal on every tick, regardless
        // of detection state. A real bike pulse is far shorter than TAU_MS, so
        // it barely nudges the baseline; but if it only updated during IDLE, a
        // sustained drift that trips the threshold would freeze baseline
        // tracking and never recover.
        if (!baselineInit) {
            baseline = smoothed;
            baselineInit = true;
            lastBaselineUpdate = now;
        } else {
            float alpha = (now - lastBaselineUpdate) / TAU_MS;
            if (alpha > 1.0f) alpha = 1.0f;
            baseline += alpha * (smoothed - baseline);
            lastBaselineUpdate = now;
        }

        bool above = smoothed > baseline + OFFSET;
        DetectionResult result = {DE_NONE, 0, 0};

        switch (state) {
            case IDLE:
                if (above && (now - lastCount >= (unsigned long)COOLDOWN_MS)) {
                    state      = IN_PULSE_1;
                    pulseStart = now;
                    pulsePeak  = (int)smoothed;
                    rawPeak    = val;
                }
                break;

            case IN_PULSE_1:
                if (above) {
                    if ((int)smoothed > pulsePeak) pulsePeak = (int)smoothed;
                    if (val > rawPeak) rawPeak = val;
                } else {
                    if (now - pulseStart >= (unsigned long)MIN_PULSE_MS) {
                        pulse1End = now;
                        state = BETWEEN;
                    } else {
                        state = IDLE;
                    }
                }
                break;

            case BETWEEN:
                if (now - pulse1End > (unsigned long)MAX_PAIR_GAP) {
                    bikeCount++;
                    lastCount = now;
                    result = {DE_UNPAIRED, pulsePeak, rawPeak};
                    state = IDLE;
                } else if (above && (now - pulse1End >= (unsigned long)MIN_PAIR_GAP)) {
                    state      = IN_PULSE_2;
                    pulseStart = now;
                    pulsePeak  = (int)smoothed;
                    rawPeak    = val;
                }
                break;

            case IN_PULSE_2:
                if (above) {
                    if ((int)smoothed > pulsePeak) pulsePeak = (int)smoothed;
                    if (val > rawPeak) rawPeak = val;
                } else {
                    if (now - pulseStart >= (unsigned long)MIN_PULSE_MS) {
                        bikeCount++;
                        lastCount = now;
                        result = {DE_PAIRED, pulsePeak, rawPeak};
                        state = IDLE;
                    } else {
                        state = (now - pulse1End <= (unsigned long)MAX_PAIR_GAP) ? BETWEEN : IDLE;
                    }
                }
                break;
        }

        return result;
    }
};
