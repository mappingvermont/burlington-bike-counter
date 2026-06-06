import pandas as pd

THRESHOLD    = 65
MIN_PULSE_MS = 5
MIN_PAIR_GAP = 200
MAX_PAIR_GAP = 1500

df = pd.read_csv('log.csv')

IDLE, IN_PULSE_1, BETWEEN, IN_PULSE_2 = 0, 1, 2, 3
state      = IDLE
pulseStart = 0
pulse1End  = 0
pulsePeak  = 0
bikeCount  = 0
events     = []

for _, row in df.iterrows():
    now   = row['ms']
    val   = row['adc']
    above = val > THRESHOLD

    if state == IDLE:
        if above:
            state      = IN_PULSE_1
            pulseStart = now
            pulsePeak  = val

    elif state == IN_PULSE_1:
        if above:
            pulsePeak = max(pulsePeak, val)
        else:
            if now - pulseStart >= MIN_PULSE_MS:
                pulse1End = now
                state = BETWEEN
            else:
                state = IDLE

    elif state == BETWEEN:
        if now - pulse1End > MAX_PAIR_GAP:
            events.append({'ms': now, 'event': 'unpaired', 'peak': pulsePeak, 'count': bikeCount})
            state = IDLE
        elif above and (now - pulse1End >= MIN_PAIR_GAP):
            state      = IN_PULSE_2
            pulseStart = now
            pulsePeak  = val

    elif state == IN_PULSE_2:
        if above:
            pulsePeak = max(pulsePeak, val)
        else:
            if now - pulseStart >= MIN_PULSE_MS:
                bikeCount += 1
                events.append({'ms': now, 'event': 'bike', 'peak': pulsePeak, 'count': bikeCount})
                state = IDLE
            else:
                # brief dip — stay in pairing window if still within MAX_PAIR_GAP
                if now - pulse1End <= MAX_PAIR_GAP:
                    state = BETWEEN
                else:
                    state = IDLE

results = pd.DataFrame(events)
print(f"Total bikes counted: {bikeCount}")
print(f"Expected:            16 (8 riding + 8 walking)")
print()
if len(results):
    print(results.to_string(index=False))
