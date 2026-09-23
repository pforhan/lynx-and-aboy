# Open Questions

Design decisions that are **not yet** settled. Each has an ID (`Q-`), a summary
of the tension, and any evidence/proposed resolution. **Settled** questions
move to [DECISIONS.md](./DECISIONS.md) (as `D-Q*`), so a question disappearing
from this file means it was decided, not forgotten. Q-9 – Q-13 moved on
2026-09-23; Q-3, Q-7, Q-14, Q-16, Q-17, Q-18, Q-20 moved on
2026-09-26 (see
[DECISIONS.md](./DECISIONS.md)).

Progress on these is tracked in [ROADMAP.md](./ROADMAP.md).

## Q-2: How much do real game authors touch the 1bpp `sBuffer` directly?

- Some use `arduboy.sBuffer[x]` to read/modify pixels without the drawing
  helpers (common for effects, HD-only tricks, third-party canvas libs).
- Others use `Sprites`, `arduboy.drawPixel`, text, and `display()` only.

The answer drives how hard we must optimize `paintScreen()` and whether a
byte-granular "dirty region" redraw is worth it - and how aggressively
ArduboyLx can skip the 1bpp layer entirely (see D-Q12 / D-Q18).

*Evidence to gather:* survey a handful of popular open-source Arduboy games
(e.g. the Arduboy collection, "Tracy+", game jam entries) and classify how
they produce each frame. See [ROADMAP.md](./ROADMAP.md) step 6.

*Survey read (2026-09-26) - 8 games classified (table in NOTES.md):*
method-calls dominate; a *few* games touch the buffer directly and mostly
through `getBuffer()`, not `sBuffer[x]` byte pokes:
- [Catecombs of the Damned](https://github.com/jhhoward/Arduboy3D) a
  graphics-intense 3d game does all its work with method calls which we can
  optimize more easily. [Platform expectations](https://github.com/jhhoward/Arduboy3D/blob/master/Source/Arduboy3D/Platform.h)
  Note that this does include access to the screen buffer array via
  `getBuffer()`.
- [ABC](https://github.com/tiberiusbrown/abc) an interpreter/compiler seems to
  access sBuffer in some [assembly](https://github.com/tiberiusbrown/abc/blob/ec76bf6d6fc3a97381594bdb72ecc6b09c224dcc/interp_arduboy/SpritesABC.cpp#L211)
  - though possibly a new abc backend could eliminate this.
- [MicroCity](https://github.com/jhhoward/MicroCity) (Sim city clone) seems to
  almost always use library methods to draw. Exception is power connectivity
  display which uses `getBuffer()`.
  [MicroCity.ino](https://github.com/jhhoward/MicroCity/blob/master/Source/MicroCity/MicroCity.ino)
  itself is the bridge to the arduboy library, translating game draw calls to
  arduboy methods.
- [DarkAndUnder](https://github.com/Garage-Collective/Dark-And-Under) doesn't
  seem to reference the sBuffer or `getBuffer()` at all, uses library methods.
  Makes a lot of use of Arduboy2's `drawCompressed`, `Sprites`, and loads from
  flash.
- [Arduventure](https://github.com/krperry/ID-46-Arduventure), a little rpg,
  does seem to use sBuffer but this is because the arduboy library is copied
  into the source tree as Arglib.h/.cpp.
- [CastleBoy](https://github.com/jlauener/CastleBoy) draws almost exclusively
  with sprites from Arduboy2 - no sBuffer references.
- [Mystic Balloon](https://github.com/Gaveno/ID-34-Mystic-Balloon) also uses
  sprites, no sBuffer.
- [Bone Shakers](https://github.com/jhhoward/BoneShakers) is a 3d kart racer
  that doesn't seem to use sBuffer, and does a lot of bitmap and direct pixel
  drawing - though interestingly sometimes this is while reading a texture as a
  source.

*Proposed direction:* the survey says method calls dominate; direct access is
concentrated and mostly via `getBuffer()`. Per
[D-Q18](./DECISIONS.md#d-q18) the priority is: fully support every draw
**method** writing straight to the Lynx buffer first, then iterate on
sBuffer/getBuffer() solutions. That largely defers the detection machinery.

*Progress so far:*

- **How can we detect sBuffer direct use?** There is a `getBuffer()`, plus
  static vs instance buffer members depending on classic Arduboy vs Arduboy2.
  Can we detect writes to it at build time (LTO write-analysis) or at runtime
  (a torn/aliased-memory marker)? Or does the survey tell us to stop worrying
  and just keep the 1bpp semantics?
  *Answer* Would it be possible to use static analysis here?  or some kind of
  #define or constexpr silliness?
- **Which detection mechanism, exactly?** The "static analysis vs `#define`
  silliness" direction needs to become concrete: (a) LTO write-analysis at
  build time, (b) a `#define`-toggled buffer type/accessor so direct
  `sBuffer[x]` writes route through one place, or (c) a runtime torn-marker.
  Decide after the survey shows how common direct `sBuffer` use actually is.
  *Deferral:* with D-Q18 ordering "methods first, sBuffer later", this is on
  the back burner until the method layer is done.
- **Proxy-class fallback (research 2026-09-23).** If direct `sBuffer` access
  turns out too common to detect cheaply, a C++ proxy `sBuffer`
  (`operator[]` returning a `ByteRef`) keeps byte-exact 1bpp semantics with
  zero source changes: reads/writes land in native Arduboy layout in RAM and
  the transpose happens as one bulk pass inside `display()` - the cost we
  already pay. This is the "stop worrying and keep 1bpp semantics" option
  made concrete; it just adds per-access call overhead when a game is
  particularly buffer-hungry. *(Crossover with the zero-transpose path is
  priced in NOTES.md.)*
- **Arduboy vs Arduboy2 API drift - settled.** *Answer* Generally the plan is
  to allow as-is source to just compile directly. So both API families are in
  scope and the survey's per-library breakout is less urgent than it seemed (see
  D-Q12). I suspect no one imports both libraries at the same time.

## Q-15: How should ArduboyG's 4-gray model map onto the palette-indexed 4bpp frame?

*Low priority:* ArduboyG is a niche (only a small subset of games use it -
[D-Q3](./DECISIONS.md#d-q3)). This mapping question sits until the step-11
reference port is attempted; meanwhile ArduboyG's method-override / phase
structure is mined for **API hints** without a port.

Research (Q-3 → now [D-Q3](./DECISIONS.md#d-q3)) shows ArduboyG owns no direct
buffer and instead manipulates the 1bpp buffer to *simulate* extra gray levels
via time-domain frames, redrawing in phases. On the Lynx, four true gray levels
quantized from the master palette could replace the timing trick - but that
changes ArduboyG's behavior from "flicker/stacked frames" to "real gray", and
it ties into Arduboy2's `display()` cadence.

*Followups:*
- **Frame/phase cadence - answered (direction).** *Answer* I think the magic it
  does is that it overrides some of the base Arduboy2 methods with its own, and
  it has a concept of phases to redraw. Game logic should only run once for all
  the frame phases, but graphics need to be hit every pass. So we may need to
  simulate that. *Open:* exactly how to simulate phase redraw on the Lynx's
  continuous-DMA display - multiple `display()` passes per game-logic tick, or
  true-gray stacking in one pass?
- **Coexistence with Arduboy2 - settled.** *Answer* it looks like you replace
  arduboy2 with arduboyg. And Sprites with SpritesU. (Closed by
  [D-Q3](./DECISIONS.md#d-q3).)
- If we map gray levels to fixed palette slots (e.g. a near-black → near-white
  ramp on 4 indices), do stock Arduboy images still look right, and do we keep
  the "true gray" mapping always-on or only when ArduboyG is detected?

## Q-19: What enhanced-surface features should ArduboyLx eventually offer Lynx-first games?

The native surface is settled: a **pointer to the back buffer** with
`display()` auto-flipping ([D-Q18](./DECISIONS.md#d-q18)). Deferred / low
priority additions from D-Q18, parked here until the method layer is done:

- **Both buffers + explicit flip.** Should enhanced games be able to read the
  *displayed* front buffer, and/or present manually (`flip()`) instead of
  piggy-backing on `display()`? Useful for effects that read the actual screen.
- **Stock single-buffer read-back.** A stock game that reads `sBuffer` back now
  sees the back buffer, whose contents can differ from the front. Does any
  common pattern break (reading then re-drawing unchanged pixels, screen-space
  sampling)?
- **Native sprite vs canvas z-order.** D-Q18 allows native 4bpp sprites and
  1bpp canvas draws in the same back buffer, but native sprites may draw out of
  order (Suzy blits). How do we guarantee ordering without a full compositor?
- *Lowest priority:* a screen-sized Suzy sprite used as a scratchpad becomes
  more appealing once the two buffers can be addressed - revisit after D-Q18.