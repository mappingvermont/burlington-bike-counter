# 2026-08-03

I put the bike counter out 'for real' for the first time this weekend. Had it out Saturday and Sunday in it's (mostly) final, hardened form:

- Pelican case
- Display fully closed with ratchet straps, etc attached to a tree
- No fewer than three locks; multiple steel locking cables

It was cool to deploy it. I knew there would be issues, but it represents a lot of work, time and thought since I first started researching this a few years ago.

As with any good demo, it started out well but then went off the rails immediately.

## Saturday

I deployed it. Rode by to check up on it two hours later and the counter displayed 2,929 bikes. Displaying absolute garbage data right above my nice "BIKES TODAY" sign - not a good look.

I came back about an hour later to take it down and found that the display had turned off. Also not a good look, but I guess better than showing the bad data. While trying to take it down I broke the key off in the cheap disc lock I was using to secure it. Eventually got it out using 3-in-1 oil and a hairpin. Just another yak-shaving moment / pain point in this real world deploy cycle!

The inflated numbers all happen in bursts - claude for some reason refers to them as "storms" - where the pressure readings spike and we get constant bike counts for a few minutes. My guess here is that it's something to do with the pressure or temp inside the pelican case.

After bringing everything home I "shipped" three fixes:
- Added an additional hole in the pelican case to get the reference sensor port out to the open air to try and mitigate the 'storms'
- Dropped the brightness on the display thinking the shut-off was a battery issue
- Bought a proper padlock :D

## Sunday

I deployed again. Came back two hours later and found that the display had turned off. I brought the display home and after unscrewing the case (ugh) found that the battery was at 91%. Still unclear what's going on here, but I guess I'm glad the battery itself is fine. More troubleshooting going on [here](display/troubleshooting.md).

I left the sensor itself out for the rest of the day to collect data. I came back Monday morning to find it removed from the path and piled up on the side. Kind of a bummer. It did collect data until 7 PM though. Analyzing the data it looks like the 'storm' problem was not fixed by exposing the reference port to the air. I'll need to dive into that. Troubleshooting is [here](phase2/troubleshooting.md).

## Reflection

### What went well

It felt great to finally get this thing out there. It's imperfect, I don't have official permission, and both the sensor and display are suffering from massive bugs. BUT I did build a thing, deploy it and have it count and display bikes (kind of)?

<img width="4032" height="3024" alt="One shining moment" src="https://github.com/user-attachments/assets/fe8458af-f4e0-448b-8cda-baf873266a52" />

More good things:
- Display mount is working really well
- Inside of Pelican case is dry despite rain last night
- I've learned so much
- I am no longer afraid of soldering

### What went poorly

- Display showed 2000+ bikes which is insane
- Hard to troubleshoot bugs with both sensor and display
- Sad to see my bike counter ripped off the pavement and coiled in the woods. I hope the sensor itself wasn't damaged.

### Overall

This has been an expensive project ~ $800 likely all told. That being said it's been a dream of mine, and it's also been very effective at giving me something to do / keeping the bad thoughts out. I think I'm OK if it ends here, given all I've tried and all I've learned. I'll likely keep going, but this is a good checkpoint regardless.


## Next steps

- Troubleshoot the sensor 'storm' issue and the display going blank
- Get official permissions / understand why someone removed the strip
- Try try again

# 2026-08-09

I've been doing some testing and observing a wild range of 'resting' pressure. Values from 30s all the way to the 700s.

I now have a debug script that just prints the ADC value every 250ms to the display screen - it's been incredibly helpful to debug in real time. Stuff like "ok, how would the pressure change if the sensor was IN the pelican case?" and "what if I add this tube to the reference port?".

My only thought here is that these spikes must be related to temperature. I was testing inside and getting pressure values in the 30s. I brought it out, put the sensor in a cardboard box and laid the tube on the hot drivway - immediately started seeing values in the 700s. It makes sense - it's a thin, black tube and the walls must change relative to temperature. Going from inside at 74 degrees to a hot driveway at 85 degrees is a big jump.

My new approach is to compare any value to the previous ~10 seconds of values. I hope that this will allow us to filter out any spikes in pressure due to temperature / wind / whatever. If a reading is +40 above that baseline, it will count as a bike. We may need to remove that magic number in favor of a ratio instead- we'll see.

I'm currently testing on the driveway wth the sensor in a cardboard box. It recorded the first 6 bike passes successfully, thank goodness. The real test now will be leaving it out for an hour and seeing if any 'phantom' bikes appear.

If this works I'll try it in the pelican case next. And if not . . . measuring bikes with this setup may not be the best approach.

# 2026-08-12

I've been testing and testing. Today I followed the latest claude testing protocal - detailed in phase2/vent_test/vent_test.ino - to try and capture sub-second changes while riding over stuff.

I've had issues with SD card writes, so now I log a 9999 (card working) and 8888 (card not working) to the display for the first few seconds, just to make sure things are set up correctly.

I mixed these values up on initial setup today. Maybe it's my time in GIS, but 9999 always seems like bad / no data. I interpreted that as SD card not working, when in reality it was working fine. I power cycled a few times until I got 8888 (the wrong value to request here, but I thought it was right). I then tested each hour - riding my bike at six places. Of course when I pulled the card . . . no data.

What a nightmare. This hardware stuff is harder than it looks! My god.

To try and salvage something from this test I will say:

- 8:47
  - 4/6 saw visible jumps in the monitor
  - unclear if those that didn't were just due to display timing? idk
  - values were around 150 at that time
- 10:09
  - values range from 300 to 330
  - saw a few jump from 330 to 360, but honestly that's not gonna cut it
- 10:46
  - values around 300
  - a few saw values jump to 400 or so
  - but also some with a +20 jump which does not seem recordable
- 11:47
  - values 25-35
  - still warm but now very cloudy instead of sun
  - SO interesting though -> at no point did any of my riding over seem to register
  - this is honestly the real issue / weirdness
- 12:47
  - same as above

After the test ended I unplugged the tube, then reseated it and was able to register visible pulses again. so interesting (shitty) that it can get into these states where it doesn't register any pressure change, even though the baseline ADC is so low (theoretically what we want). Man.


