# Open Questions

Design decisions that are **not yet** settled. Each has an ID (`Q-`), a summary
of the tension, and any evidence/proposed resolution. **Settled** questions
move to [DECISIONS.md](./DECISIONS.md) (as `D-Q*`), so a question disappearing
from this file means it was decided, not forgotten. Q-9 – Q-13 moved on
2026-09-23 (see [DECISIONS.md](./DECISIONS.md)).

Progress on these is tracked in [ROADMAP.md](./ROADMAP.md).

## Q-2: How much do real game authors touch the 1bpp `sBuffer` directly?

- Some use `arduboy.sBuffer[x]` to read/modify pixels without the drawing
  helpers (common for effects, HD-only tricks, third-party canvas libs).
- Others use `Sprites`, `arduboy.drawPixel`, text, and `display()` only.

The answer drives how hard we must optimize `paintScreen()` and whether a
byte-granular "dirty region" redraw is worth it — and how aggressively
ArduboyLx can skip the 1bpp layer entirely (see D-Q12).

*Evidence to gather:* survey a handful of popular open-source Arduboy games
(e.g. the Arduboy collection, "Tracy+", game jam entries) and classify how
they produce each frame. See [ROADMAP.md](./ROADMAP.md) step 6.

*Proposed direction:* a survey is definitely needed; many games likely hit
the 1bpp buffer often. Prefer a library design that *skips* that layer when
possible.

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
- **Proxy-class fallback (research 2026-09-23).** If direct `sBuffer` access
  turns out too common to detect cheaply, a C++ proxy `sBuffer`
  (`operator[]` returning a `ByteRef`) keeps byte-exact 1bpp semantics with
  zero source changes: reads/writes land in native Arduboy layout in RAM and
  the transpose happens as one bulk pass inside `display()` — the cost we
  already pay. This is the "stop worrying and keep 1bpp semantics" option
  made concrete; it just adds per-access call overhead when a game is
  particularly buffer-hungry.
  *Survey* 
  * [Catecombs of the Damned](https://github.com/jhhoward/Arduboy3D) a graphics-intense 3d game does all its work with method calls which we can optimize more easily.  [Platform expectations](https://github.com/jhhoward/Arduboy3D/blob/master/Source/Arduboy3D/Platform.h) Note that this does include access to the screen buffer array via getBuffer()
  * [ABC](https://github.com/tiberiusbrown/abc) an interpreter/compiler seems to access sbuffer in some [assembly](https://github.com/tiberiusbrown/abc/blob/ec76bf6d6fc3a97381594bdb72ecc6b09c224dcc/interp_arduboy/SpritesABC.cpp#L211) -- though possibly a new abc backend could eliminate this
  * [MicroCity](https://github.com/jhhoward/MicroCity) (Sim city clone) seems to almost always use library methods to draw. Exception is power connectivity display which uses getBuffer(). [MicroCity.ino](https://github.com/jhhoward/MicroCity/blob/master/Source/MicroCity/MicroCity.ino) itself is the bridge to the arduboy library, translating game draw calls to arduboy methods.
  * [DarkAndUnder](https://github.com/Garage-Collective/Dark-And-Under) doesn't seem to reference the sbuffer or getBuffer() at all, uses library methods.  Makes a lot of use of Arduboy2's drawCompressed, Sprites, and I may have seen a reference to loading from flash
  * [Arduventure](https://github.com/krperry/ID-46-Arduventure), a little rpg, does seem to use sbuffer but this is because the arduboy library is copied into the source tree as Arglib.h/.cpp
  * [CastleBoy](https://github.com/jlauener/CastleBoy) draws almost exclusively with sprites from Arduboy2 -- no sbuffer references.
  * [Mystic Balloon](https://github.com/Gaveno/ID-34-Mystic-Balloon) also uses sprites, no sbuffer
  * [Bone Shakers](https://github.com/jhhoward/BoneShakers) is a 3d kart racer that doesn't seem to use sbuffer, and does a lot of bitmap and direct pixel drawing -- though interestingly sometimes this is while reading a texture as a source.

- **Arduboy vs Arduboy2 API drift.** Classic `Arduboy` and `Arduboy2` expose
  `sBuffer` differently (static vs instance). Does ArduboyLx target both, and
  does the survey's data break out by library version?
  *Answer* Generally the plan is to allow as-is source to just compile directly.
  So both API families are in scope and the survey's per-library breakout is
  less urgent than it seemed (see D-Q12).  I suspect no one imports both 
  libraries at the same time, though.

## Q-3: Should we support ArduboyG (4-color, high FPS) natively?

ArduboyG drives the panel with 4 distinct gray levels using a high frame rate
instead of 1bpp. Our 4bpp Lynx backend could map the same 4 "gray" values
onto 4-color Lynx palette entries directly. Is it worth maintaining a separate
engine path, or is a per-game port a better template? This is also a good
minimal example for how a third party would write a Lynx-specific library
against our shim.

*Proposed direction:* support it natively, delegating to the upstream
ArduboyG when building for the Arduboy platform — nearly a superset-of-
libraries story.

*Progress so far (answer 2026-09-23):*

- **Research the actual ArduboyG API surface** before committing a Lynx
  substrate: does it own the display buffer? Can it coexist with Arduboy2 in
  one sketch (display ownership conflict)?
  *Answer* A quick glance doesn't look like it exposes a direct buffer. It
  manipulates the 1bpp buffer to simulate extra colors. So ArduboyG works *on
  top of* the existing 1bpp semantics — our shim can host it without a new
  buffer model. How its 4-gray timing maps to the Lynx is still open
  ([Q-15](#q-15)) and its `display()` cadence vs. Arduboy2's needs a same-
  sketch coexistence check.
- **Substrate choice:** map 4 gray levels onto 4 of the 16 palette entries, or
  onto a 2bpp *display* path?
  *Answer* Lynx screen buffer is always a palette-driven 4bpp; 4-color (2bpp)
  is just a sprite color-depth option. Substrate therefore = 4 master-palette
  slots on the 4bpp frame (settled by [D-Q10](./DECISIONS.md)).
- **Is native support worth a dedicated engine path?** Still open. The
  reference port (ROADMAP step 11) decides: does ArduboyG-on-Lynx reach
  the D-Q6 frame-rate target and look right without a bespoke path?

## Q-7: GRB color order and emulator/hardware divergence

The Lynx 4bpp framebuffer and its color model need a definitive checklist
across real hardware and at least two emulator cores before trusting any
single renderer's colors. See [I-1](KNOWN_ISSUES.md#i-1).

*Answer (2026-09-23) — corrects the framebuffer model:* The display buffer
itself is all 4-bit palette indices, not direct colors.
How You Pick and Define a Color: The Lynx features a 12-bit master palette (4,096 colors).
To pick and define a color, you write 4-bit intensity values (0 to 15, or 0x0 to 0xF) into
two separate sets of 16-byte hardware registers mapped in memory:
- Green Channel ($FDA0 to $FDAF): The 16 registers that hold the 4-bit Green value for each
  palette index.
- Blue & Red Channels ($FDB0 to $FDBF): The 16 registers that hold both the Blue and Red
  values for each index.
The Hardware Byte Layout
When writing to hardware memory to assign a color to a palette index:
- Green Entry: Store your 4-bit green value into the lower nibble of the corresponding
  address between 0xFDA0 + index.
- Red/Blue Entry: Store your 4-bit red and blue values into a single byte at 0xFDB0 +
  index. The byte layout requires the upper nibble to be Red and the lower nibble to be
  Blue (or vice versa depending on how your code swaps them for the chip).
For example, to set Palette Slot 3 to a specific shade of purple, you would write the Green intensity to $FDA3 and the packed Red/Blue intensities to $FDB3.

*Consequence for I-1:* the renderer currently writes raw GRB values
(`0x07`, `richPal[]`, `Arduboy2Core.cpp`) into the framebuffer *as if* they
were direct colors — but they are palette indices. If the default palette does
not map those indices to the intended colors, that alone explains
"everything renders green". ROADMAP step 5 must program the master palette and
switch to index writes.

*Followups:*
- **Default vs programmed palette.** What does the default master palette map
  each index to on real silicon and in each emulator core? Program slots 0 and 7
  explicitly for B/W (plus the rich-palette slots)
  *Answer* I recommend presenting a few fun palettes we can cycle via menu or option button press.  If enhanced (using AdruboyLx directly), let the game drive the default, otherwise Black/white/two grays as default but we could define a few others, like different monochromes or cga color sets, etc.
- **Register packing.** Whether the upper/lower nibbles of `$FDB0+i` are
  Red/Blue or Blue/Red is cosmetic because both channels share the byte — but
  codify the exact packing (per emulator core) in one palette-init routine and
  re-record it in NOTES.md.
- **Where does the palette live?** Palette registers vs. hacks like
  `SPRSYS`/sprite `TST` colors — the master palette is also what Suzy sprites
  use (D-Q9), so a single palette-init function should serve both renderers.

## Q-14: How do Suzy's native sprite features map onto the upstream `Sprites` API?

[D-Q9](./DECISIONS.md) commits to driving Suzy's SPRDISP/SPRCTL path for the
`Sprites` library while keeping `SpritesLynx.cpp` as the software fallback.
Suzy has its own color machinery: 1bpp sprites draw with two colors selected
from the master palette, 2bpp/4bpp sprites carry palette indices in the data,
and SPRSYS / GSPR_TST registers handle transparent color and collision. The
upstream `Sprites` API carries flags (`H_FLIP`, `V_FLIP`, `CLEAR`, `ALPHA`)
and draw modes (`PS_MASKED`, `PS_OR`, `PS_AND`, `PS_XOR`, ...) that must stay
faithful.

*Followups:*

- **Pre-convert at build time (favored).** Static `const uint8_t xx[] PROGMEM`
  sprite arrays can be transposed to Lynx scanline format *at compile time* — a
  `constexpr LynxSprite` wrapper with an implicit `operator const uint8_t*`,
  or a macro-injected declaration — so Suzy blits straight from ROM with zero
  runtime transpose (see NOTES.md). Decide the wrapper mechanics and whether
  `-include` injection keeps game source byte-identical, and whether 8px-band
  stitching handles >8px-tall sprites.
- **Palette mapping.** When a 1bpp Arduboy sprite is drawn, which two
  master-palette indices does Suzy select, and how do we make those honor the
  current ink / rich-palette state without reprogramming the master palette
  mid-frame? Can a fixed B/W index pair (or a per-sprite palette offset) keep
  `Sprites` flags byte-faithful?
  *Answer* Default to the palette of the ardu screen area.  Consider adding 
  enhancements to ArduboyLx to provide more options.
- **Mixed CPU/Suzy render.** The game window also receives `paintScreen()`
  fills, text, and native draws. Do Suzy sprite blits go through the same back
  buffer so the DISPADR swap stays consistent, or can they render directly?
  *Answer* We should always write to the back buffer.
- **Verification.** How do we diff a Suzy-rendered frame against the software
  `SpritesLynx.cpp` reference to catch semantics drift (golden-frame harness,
  ROADMAP step 4)?
  *Answer* I think the golden-frame harness may be overkill.

## Q-15: How should ArduboyG's 4-gray model map onto the palette-indexed 4bpp frame?

Research (Q-3) shows ArduboyG owns no direct buffer and instead manipulates
the 1bpp buffer to *simulate* extra gray levels via time-domain frames. On the
Lynx, four true gray levels quantized from the master palette could replace
the timing trick — but that changes ArduboyG's behavior from "flicker/stacked
frames" to "real gray", and it ties into Arduboy2's `display()` cadence.

*Followups:*
- Does upstream ArduboyG depend on a specific `display()`/frame-stacking
  cadence that the Lynx's continuous-DMA display complicates, and can the Lynx
  satisfy its implied extra frame rate without doubling CPU work?
  *Answer* I think the magic it does is that it overrides some of the base 
  Arduboy2 methods with its own, and it has a concept of phases to redraw.
  Game logic should only run once for all the frame phases, but graphics need
  to be hit every pass.  So we may need to simulate that.
- If we map gray levels to fixed palette slots (e.g. a near-black → near-white
  ramp on 4 indices), do stock Arduboy images still look right, and do we keep
  the "true gray" mapping always-on or only when ArduboyG is detected?
- Does ArduboyG coexist with Arduboy2 in one sketch on the Lynx (who owns
  `display()` / the swap)?
  *Answer* it looks like you replace arduboy2 with arduboyg.  And Sprites with
  SpritesU.

## Q-16: What replaces the Arduboy FX chip's asset flash on the Lynx?

[D-Q11](./DECISIONS.md) adds a build-time guard for FX-backed assets, but it
does not say what to do with games that legitimately depend on the FX ROM's
4 MB for graphics/sound. BLL images are RAM-resident; nothing like the FX
`readProgram`/`readBytes` streaming exists on the Lynx.

*Followups:*
- Is **cartridge-ROM** support (a ComLynx cartridge image bigger than RAM,
  bankswitched or paged) in scope, or is the answer a per-game port that
  compresses assets into the 32 KB-class image?
  *Answer* this may have to be a per-game build flag, defaulting to squishing 
  everything into something that fits in the ~48kb we have. Reading more off 
  the cart will be out of scope for non-modified games for now.
- For the launcher / multi-title idea ([D-Q4](./DECISIONS.md)): would the
  launcher live in the same ROM image, and does that change the FX substitute?
  *Answer* Same lynx ROM image.
- What is the simplest author-facing detection: a build-time link/`objcopy`
  analysis that flags oversized PROGMEM arrays and any FX-API calls, plus an
  `#ifdef __LYNX__` knob for "I don't use FX"?
  *Answer* at this time just build-time analysis of size. We could even defer all
  fx support until later and just let compilation fail.

## Q-17: Is DISPCTL's 2-bit (4-color) display mode real, and is it ever worth using?

[D-Q10](./DECISIONS.md) standardizes on 4bpp and treats 2bpp as a sprite
color-depth option, but MonLynx docs and NOTES.md document `DISPCTL` B2 as
selecting a 2-bit *display* mode (the `0x05` mono-2bit value in
[NOTES.md](./NOTES.md)). If real, a 2-bit display path could halve
`paintScreen()`'s expansion work and buy frame-rate headroom (D-Q6) for pure
B/W games — or it could be a dead end.

*Answer* Yes, it looks like you can force a 2-bit mode which shrinks our buffers and provides more RAM for program/graphics. Presumably byte translation and manipulation would be quicker as well. This would be a good default for unmodified programs.

*Followups:*
- Verify 2-bit mode on silicon/emulator: does `DISPCTL = 0x05` actually show
  2-bit pixels (40-byte scanlines, ~4080-byte framebuffer) and refresh faster?
  *Answer* Let's assume this does work.  We can double-check it when we try this mode.
- If real, does the 1bpp→2bpp expansion survive the border/centering layout
  without a separate path, and is it worth the dual-path complexity?
  *Clarify* Please clarify this, or ask another way.  If you're talking about the c++ templates and constexprs we may just need build flags to pick impls.

## Q-18: How do the native 4bpp and 1bpp drop-in paths coexist inside one ArduboyLx?

[D-Q12](./DECISIONS.md) exposes a native 4bpp framebuffer; D-Q1/G-1 keep a
1bpp drop-in for stock sketches. Q-2's "skip the layer" tension: a Lynx-first
game should skip the 1bpp stage entirely, while stock games must keep
byte-exact `sBuffer` semantics.

*Followups:*
- **Accessor shape.** If the buffer is exposed via a method, is it a pointer
  to the back framebuffer, or `fbSetPixel(x,y,index)` / `fbBurst(...)`
  primitives? How does a game signal "present this frame" (`flip()` vs. reusing
  `display()`)?
  *Answer* It will always point at the back buffer and our display() code will do the flip automatically. Something tricky is that arduboy games have a single buffer so they can read data out of them but now they could be different. This may make our experiment to use a screen-sized sprite as a scratchpad more appealing. We can make a deferred item to allow accessing both buffers and / or flipping them for enhanced games but it's low priority.
- **Mixing.** Can one sketch draw a 1bpp canvas and then native 4bpp sprites/
  text into the same frame? 
  *Answer* We should support simultaneous direct canvas writes and sprites. That may get tricky if we treat them as native sprites, where they may draw out of order with the canvas.
- **mass updates** What happens to Arduboy2's `clear()` / `fillScreen()` between native draws?
  *Answer* for now I'd map these to the same operations on the lynx buffer
- **No-cheap-way-out trap.** Does the 1bpp→4bpp expansion still run when only
  native draws happen, and how do we avoid double-clearing the back buffer?
  *Answer* expansion behavior will eventually depend on a build flag ARDUBOYLX_PAINTPATH documented elsewhere. Until then we'll keep moving 1bpp data to lynx's buffer.
- **Zero-transpose paint path (proposal 2026-09-23).** Instead of expanding
  sBuffer *at display time*, reimplement the drawing API (`drawPixel`,
  `drawBitmap`, `Sprites`, text, `fillRect`, …) to write 4bpp-native nibbles
  into the back framebuffer, with `(19+dy)*80 + (8 + (dx>>1))` and the 128×64
  clip baked as compile-time constants — then `display()` is a bare DISPADR
  swap and the transpose vanishes for **API-only games**. Where this splits:
  (a) the *geometry* is build-time folded, but sBuffer *contents* stay runtime
  (the game mutates them live) so only never-changing art (logos, tiles) is
  fully pre-baked; (b) games that poke `sBuffer[x]`/`getBuffer()` need a proxy
  shim whose byte-write fans out to eight nibble writes (and byte-read gathers
  eight back) — the Q-2 fork resurfacing as the per-access cost — or confine
  them to a software-1bpp path; (c) clearing the 128×64 window costs ~4 KB of
  nibbles vs 1 KB at 1bpp (memset-able); chrome (D-Q13) is outside the window
  and untouched. Decide whether this replaces or coexists with the Suzy
  full-screen-blit experiment (ROADMAP step 7). **Selection mechanism:** a
  build flag `-DARDUBOYLX_PAINTPATH={one-shot|native|measure}` (projected in
  ROADMAP step 6) — **default `one-shot`**; native wins for API-driven and
  light-poke games per the crossover criterion in NOTES.md; `measure`
  instruments `display()` frame time (Step 1/step 7 benchmarking).
  *Answer* First priority is full support of all methods directly writing to the lynx buffer. Then we can iterate on solutions for sBuffer / getBuffer() 