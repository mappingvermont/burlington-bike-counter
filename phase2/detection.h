#pragma once
#include <stdint.h>

const int THRESHOLD    = 65;
const int MIN_PULSE_MS = 2;
const int MIN_PAIR_GAP = 70;
const int MAX_PAIR_GAP = 500;
const int COOLDOWN_MS  = 200;

enum DetectionEvent { DE_NONE, DE_UNPAIRED, DE_PAIRED };
struct DetectionResult { DetectionEvent type; int peak; int trough; };
enum State { IDLE, IN_PULSE_1, BETWEEN, IN_PULSE_2 };

struct Detection {
    State state              = IDLE;
    unsigned long pulseStart = 0;
    unsigned long pulse1End  = 0;
    unsigned long lastCount  = 0;
    int pulsePeak            = 0;
    int trough               = 4095;
    int bikeCount            = 0;

    DetectionResult tick(unsigned long now, int val) {
        bool above = val > THRESHOLD;
        DetectionResult result = {DE_NONE, 0, 0};

        if (val < trough) trough = val;

        switch (state) {
            case IDLE:
                if (above && (now - lastCount >= (unsigned long)COOLDOWN_MS)) {
                    state      = IN_PULSE_1;
                    pulseStart = now;
                    pulsePeak  = val;
                }
                break;

            case IN_PULSE_1:
                if (above) {
                    if (val > pulsePeak) pulsePeak = val;
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
                    result = {DE_UNPAIRED, pulsePeak, trough};
                    trough = val;
                    state = IDLE;
                } else if (above && (now - pulse1End >= (unsigned long)MIN_PAIR_GAP)) {
                    state      = IN_PULSE_2;
                    pulseStart = now;
                    pulsePeak  = val;
                }
                break;

            case IN_PULSE_2:
                if (above) {
                    if (val > pulsePeak) pulsePeak = val;
                } else {
                    if (now - pulseStart >= (unsigned long)MIN_PULSE_MS) {
                        bikeCount++;
                        lastCount = now;
                        result = {DE_PAIRED, pulsePeak, trough};
                        trough = val;
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
