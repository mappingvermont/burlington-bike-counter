import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

df = pd.read_csv('log.csv')

noise_floor = df['adc'].median()
noise_std = df['adc'].std()
peak = df['adc'].max()

print(f"Samples:     {len(df):,}")
print(f"Duration:    {df['ms'].max()/1000:.1f}s")
print(f"Sample rate: {len(df)/(df['ms'].max()/1000):.0f} Hz")
print(f"Noise floor: {noise_floor:.1f} (median), std: {noise_std:.2f}")
print(f"Peak ADC:    {peak}")

# --- find events with minimum duration and peak filters ---
THRESHOLD   = 65      # ADC — comfortably above noise (34 + ~8σ)
MIN_PEAK    = 70      # must reach this within the event
MIN_SAMPLES = 30      # ~9ms at 3.4kHz — filters single-sample noise

df['above'] = df['adc'] > THRESHOLD
df['event_id'] = (df['above'] & ~df['above'].shift(1, fill_value=False)).cumsum()
df.loc[~df['above'], 'event_id'] = 0

raw_events = []
for eid, group in df[df['event_id'] > 0].groupby('event_id'):
    raw_events.append({
        'start_ms': group['ms'].iloc[0],
        'end_ms':   group['ms'].iloc[-1],
        'duration': group['ms'].iloc[-1] - group['ms'].iloc[0],
        'peak':     group['adc'].max(),
        'samples':  len(group),
    })

events = pd.DataFrame(raw_events)
events = events[(events['peak'] >= MIN_PEAK) & (events['samples'] >= MIN_SAMPLES)].reset_index(drop=True)

print(f"\nFiltered events (peak≥{MIN_PEAK}, samples≥{MIN_SAMPLES}): {len(events)}")
print(events.to_string(index=False))

# --- pair front/rear wheel pulses (gap < 2000ms) ---
events['gap_to_next'] = events['start_ms'].shift(-1) - events['end_ms']
front_pulses = events[events['gap_to_next'] < 2000].copy()

print(f"\nBike passes detected (paired pulses): {len(front_pulses)}")
print(front_pulses[['start_ms','peak','gap_to_next','duration']].to_string(index=False))

# --- plots ---
fig = plt.figure(figsize=(16, 12))
gs = gridspec.GridSpec(3, 2, hspace=0.45, wspace=0.3)

# full trace
ax1 = fig.add_subplot(gs[0, :])
ax1.plot(df['ms'] / 1000, df['adc'], linewidth=0.2, color='steelblue', alpha=0.7)
ax1.axhline(noise_floor, color='gray',  linestyle='--', linewidth=0.8, label=f'noise floor ({noise_floor:.0f})')
ax1.axhline(THRESHOLD,   color='red',   linestyle='--', linewidth=0.8, label=f'threshold ({THRESHOLD})')
for _, e in events.iterrows():
    ax1.axvspan(e['start_ms']/1000, max(e['end_ms']/1000, e['start_ms']/1000 + 0.05),
                alpha=0.3, color='orange')
ax1.set_xlabel('Time (s)')
ax1.set_ylabel('ADC')
ax1.set_title('Full trace — orange = detected events')
ax1.legend()

# ADC histogram
ax2 = fig.add_subplot(gs[1, 0])
ax2.hist(df['adc'], bins=120, color='steelblue', edgecolor='none')
ax2.axvline(noise_floor, color='gray', linestyle='--', label='noise floor')
ax2.axvline(THRESHOLD,   color='red',  linestyle='--', label='threshold')
ax2.set_xlabel('ADC value')
ax2.set_ylabel('Count (log)')
ax2.set_yscale('log')
ax2.set_title('ADC distribution')
ax2.legend()

# peak values per event
ax3 = fig.add_subplot(gs[1, 1])
ax3.bar(range(len(events)), events['peak'], color='steelblue')
ax3.axhline(MIN_PEAK, color='red', linestyle='--', label=f'min peak ({MIN_PEAK})')
ax3.set_xlabel('Event #')
ax3.set_ylabel('Peak ADC')
ax3.set_title('Peak per event (riding → walking)')
ax3.legend()

# zoom on first clean pass
if len(events):
    ax4 = fig.add_subplot(gs[2, 0])
    e = events.iloc[0]
    pad = 1000
    zoom = df[(df['ms'] > e['start_ms'] - pad) & (df['ms'] < e['end_ms'] + pad + 800)]
    ax4.plot(zoom['ms'], zoom['adc'], linewidth=0.8, color='steelblue')
    ax4.axhline(THRESHOLD, color='red', linestyle='--', label='threshold')
    ax4.set_xlabel('Time (ms)')
    ax4.set_ylabel('ADC')
    ax4.set_title(f'First pass zoom (peak={e["peak"]})')
    ax4.legend()

# front/rear gap distribution
if len(front_pulses):
    ax5 = fig.add_subplot(gs[2, 1])
    ax5.hist(front_pulses['gap_to_next'].dropna(), bins=20, color='steelblue', edgecolor='none')
    ax5.set_xlabel('Front→rear gap (ms)')
    ax5.set_ylabel('Count')
    ax5.set_title('Front/rear wheel gap distribution')

plt.savefig('analysis.png', dpi=150, bbox_inches='tight')
print("\nPlot saved to analysis.png")
