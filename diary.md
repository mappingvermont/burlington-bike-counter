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
