# Open Questions

Design decisions that are not yet settled. Each has an ID (`Q-`), a summary
of the tension, and any evidence/proposed resolution. Mark a question resolved
in place with a short `Status:` line rather than deleting it, so the history of
why the decision was made is preserved.

Progress on these is tracked in [ROADMAP.md](./ROADMAP.md).

## Q-1: What is the best target for "seamless" porting?

G-1 says "minimal or zero code changes," but the ecosystem has several entry
points: `Arduboy2Base` (drawing/text), `Sprites`, `ArduboyTones`, direct
`sBuffer` access, and `ArduboyG`'s 4-color mode. How far down the stack must
we go to claim "seamless," and what's the acceptance test?

*Proposed decision:* define "seamless" as *any sketch that builds unmodified
on a stock Arduboy also builds unmodified for the Lynx and is playable.*
Anything requiring an `#ifdef __LYNX__` line means we ship a wrapper/shim
rather than a drop-in.

## Q-2: How much do real game authors touch the 1bpp `sBuffer` directly?

- Some use `arduboy.sBuffer[x]` to read/modify pixels without the drawing
  helpers (common for effects, HD-only tricks, third-party canvas libs).
- Others use `Sprites`, `arduboy.drawPixel`, text, and `display()` only.

The answer drives how hard we must optimize `paintScreen()` and whether a
byte-granular "dirty region" redraw is worth it.

*Evidence to gather:* survey a handful of popular open-source Arduboy games
(e.g. the Arduboy collection, "Tracy+", game jam entries) and classify how
they produce each frame. See [ROADMAP.md](./ROADMAP.md) step 6.

## Q-3: Should we support ArduboyG (4-color, high FPS) natively?

ArduboyG drives the panel with 4 distinct gray levels using a high frame rate
instead of 1bpp. Our 4bpp Lynx backend could map the same 4 "gray" values
onto a 4-color Lynx palette directly. Is it worth maintaining a separate
engine path, or is a per-game port a better template? This is also a good
minimal example for how a third party would write a Lynx-specific library
against our shim.

## Q-4: What belongs in the border / chrome around the 128x64 window?

The Lynx panel is 160x102, leaving a 16px-wide / 19px-tall band around the
game window. Options: nothing (clean B/W like an Arduboy), a static frame, a
game-supplied title/score readout, or system hints ("Option 1 = palette",
"Pause = menu"). Whatever we do must be cheap per-frame and not interfere with
the game window. There's also the open question of *who* controls it: the
game decides, or we draw system chrome automatically?

Relevant to [I-4](KNOWN_ISSUES.md#i-4).

## Q-5: Is the current audio model faithful enough?

We map `BeepPin1/2` to Lynx square-wave channels A/B. Real games often use
`ArduboyTones` (a separate MLXXXp library) for richer sound. Decision pending
on whether to port `ArduboyTones` to Lynx (currently a deferred follow-up) and
whether mono/single-channel is acceptable for a first cut.

## Q-6: Can we get the frame rate *predictably* to 59.9 FPS on 4 MHz?

The 1bpp→4bpp expansion happens in software every frame. We don't yet know
the real wall-clock cost on hardware vs. the emulator's 5 FPS. The question
is the *target*: necessarily 59.9, or is a steady, capped rate (the demo's
`setFrameRate(30)` suggests 30 FPS may be acceptable) enough for most games?
Interacts strongly with [I-2](KNOWN_ISSUES.md#i-2).

## Q-7: GRB color order and emulator/hardware divergence

The Lynx 4-bit color is GRB+I: bit0=blue, bit1=red, bit2=green, bit3=ink.
Different emulator cores historically approximate palettes differently. We
need a definitive checklist across real hardware and at least two emulator
cores before trusting any single renderer's colors. See
[I-1](KNOWN_ISSUES.md#i-1).

## Q-8: What does "real hardware verified" require?

Hardware verification needs the BLL loader, a ComLynx cable, and a Lynx — a
real barrier for most contributors. Do we invest in a friendlier path (e.g. a
cartridge/Rom vector image) or keep hardware as a maintainer-only step with
emulators as the primary CI target?