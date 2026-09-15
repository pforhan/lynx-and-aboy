# Open Questions

Design decisions that are not yet settled. Each has an ID (`Q-`), a summary
of the tension, and any evidence/proposed resolution. **Settled** questions
move to [DECISIONS.md](./DECISIONS.md) (as `D-Q*`), so a question disappearing
from this file means it was decided, not forgotten.

Progress on these is tracked in [ROADMAP.md](./ROADMAP.md).

## Q-2: How much do real game authors touch the 1bpp `sBuffer` directly?

- Some use `arduboy.sBuffer[x]` to read/modify pixels without the drawing
  helpers (common for effects, HD-only tricks, third-party canvas libs).
- Others use `Sprites`, `arduboy.drawPixel`, text, and `display()` only.

The answer drives how hard we must optimize `paintScreen()` and whether a
byte-granular "dirty region" redraw is worth it — and how aggressively
ArduboyLx can skip the 1bpp layer entirely (see Q-12).

*Evidence to gather:* survey a handful of popular open-source Arduboy games
(e.g. the Arduboy collection, "Tracy+", game jam entries) and classify how
they produce each frame. See [ROADMAP.md](./ROADMAP.md) step 6.

*Proposed direction:* a survey is definitely needed; many games likely hit
the 1bpp buffer often. Prefer a library design that *skips* that layer when
possible.

*Followups:*

- **How can we detect sBuffer direct use?** There is a `getBuffer()`, plus
  static vs instance buffer members depending on classic Arduboy vs Arduboy2.
  Can we detect writes to it at build time (LTO write-analysis) or at runtime
  (a torn/aliased-memory marker)? Or does the survey tell us to stop worrying
  and just keep the 1bpp semantics?
- **Arduboy vs Arduboy2 API drift.** Classic `Arduboy` and `Arduboy2` expose
  `sBuffer` differently (static vs instance). Does ArduboyLx target both, and
  does the survey's data break out by library version?

## Q-3: Should we support ArduboyG (4-color, high FPS) natively?

ArduboyG drives the panel with 4 distinct gray levels using a high frame rate
instead of 1bpp. Our 4bpp Lynx backend could map the same 4 "gray" values
onto a 4-color Lynx palette directly. Is it worth maintaining a separate
engine path, or is a per-game port a better template? This is also a good
minimal example for how a third party would write a Lynx-specific library
against our shim.

*Proposed direction:* support it natively, delegating to the upstream
ArduboyG when building for the Arduboy platform — nearly a superset-of-
libraries story.

*Followups:*

- **Research the actual ArduboyG API surface** before committing a Lynx
  substrate: does it own the display buffer? Can it coexist with Arduboy2 in
  one sketch (display ownership conflict)?
- **Substrate choice:** map 4 gray levels onto 4 of the 16 GRB colors in 4bpp,
  or onto the Lynx's 2bpp (4-color) display mode, which refreshes faster —
  see Q-10.

## Q-7: GRB color order and emulator/hardware divergence

The Lynx 4-bit color is GRB+I: bit0=blue, bit1=red, bit2=green, bit3=ink.
Different emulator cores historically approximate palettes differently. We
need a definitive checklist across real hardware and at least two emulator
cores before trusting any single renderer's colors. See
[I-1](KNOWN_ISSUES.md#i-1).

---

## Q-9: Should sprite rendering use Suzy's hardware sprite engine?

The Lynx's Suzy chip does hardware sprites (blitting, collision, scaling,
tiling) independently of the CPU. Right now `SpritesLynx.cpp` is a portable C
reimplementation that spends CPU cycles drawn from the same budget as the
1bpp→4bpp expansion (see [I-2](KNOWN_ISSUES.md#i-2)). Do we offload sprite
drawing to Suzy's SPRDISP/SPRCTL path (and keep collision info from Suzy's
collision registers) instead? The payoff could be large for frame rate, but
it forks the sprite pipeline from upstream Sprites semantics and is a big
new surface to test.

*Tension:* G-2 (keep the engine verbatim) vs. G-6/I-2 (make it fast and
seamless).

## Q-10: Which Lynx display modes should we drive — 4bpp now, 2bpp later?

The Lynx panel has multiple modes: 4bpp (16 colors, our current
[DISPCTL](NOTES.md)), and 2bpp (4 colors) which the hardware drives at a
higher refresh. For a 1bpp Arduboy there's little reason to leave 4bpp; but
for ArduboyG's 4-gray model (Q-3) and for frame-rate headroom (D-Q6), 2bpp
may be the natural substrate. Decision needed: standardize on 4bpp and map
gray levels onto palette entries, or add a 2bpp path for speed when color
count permits?

## Q-11: What's the realistic size/memory ceiling for a Lynx game image?

The BLL image loads into RAM (64 KB total, less two 8160-byte framebuffers
and the running program), unlike an Arduboy whose PROGMEM stays in flash.
Large data arrays and big `static const` assets that Arduboy game authors
take for granted may not fit. How do we (a) measure and document the ceiling,
(b) warn authors, and (c) decide whether cartridge-ROM images (bigger than
RAM) are in scope? Also affects the multi-title launcher idea in
[D-Q4](./DECISIONS.md).

## Q-12: Should ArduboyLx expose a native framebuffer, or mirror the 1bpp API?

D-Q1 says ArduboyLx may describe everything the Lynx can do while delegating
to Arduboy libraries on the Arduboy side. On the Lynx side, that argues for a
native API that renders straight to 4bpp (*no* 1bpp buffer at all) for games
written Lynx-first — while the drop-in mode (G-1) still honors 1bpp semantics
for stock Arduboy games. Do we design ArduboyLx around a native 4bpp canvas
with a 1bpp compatibility mode, or keep the 1bpp stage as the single source of
truth (Q-2's "skip the layer" tension)? This is the biggest architectural fork
in the ArduboyLx design.

## Q-13: Who draws the chrome, and at what cost?

[D-Q4](./DECISIONS.md) settles *what* the chrome shows (title strip, boxed
pause, settings) but not *who* owns it or how cheaply it's produced. Options:

- **Framework-driven:** ArduboyLx/system draws chrome planes automatically;
  the game never thinks about it. Clean for stock games, but constrains a
  title to what the system knows.
- **Game-driven:** expose a chrome-drawing API; the game fills its own
  title/status. Flexible, but stock games show nothing and per-frame cost is
  the game's problem.
- **Hybrid:** system draws chrome once into a separate plane/tile that
  survives game clear+repaint (relevant to I-4 and ROADMAP step 8's "stamp it
  once" idea).

Interacts with the multi-title launcher idea in
[D-Q4](./DECISIONS.md).