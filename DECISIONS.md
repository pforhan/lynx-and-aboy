# Decisions

Settled design decisions, moved here from
[OPEN_QUESTIONS.md](./OPEN_QUESTIONS.md). Each keeps its original `Q-` id
(e.g. **D-Q1** was formerly `Q-1`) so the rest of the docs can reference the
decision history without ambiguity. The rationale and any dissent are recorded
here; the *effect* on code, issues, and roadmap is disseminated in the file
each decision points to.

Date recorded: 2026-09-08. Additional decisions recorded 2026-09-23
(D-Q9, D-Q10, D-Q11, D-Q12, D-Q13) and 2026-09-26 (D-Q3, D-Q7, D-Q14,
D-Q16, D-Q17, D-Q18, D-Q20).

## D-Q1: Ship an "ArduboyLx" API layer (was Q-1)

- **Question:** What is the best target for "seamless" porting?
- **Decision:** Adopt the proposed acceptance test — *any sketch that builds
  unmodified on a stock Arduboy also builds unmodified for the Lynx and is
  playable.* Provide an **ArduboyLx** API (following the ArduboyG pattern)
  that falls back to standard Arduboy semantics on the Arduboy platform.
  Advanced features (sprites, etc.) are added to ArduboyLx over time but keep
  delegating to suitable Arduboy native libraries where that makes sense. It
  is possible that ArduboyLx describes everything the Lynx can do while
  delegating to many native Arduboy libraries when compiling for the Arduboy.
- **Effect:** [G-1](./GOALS.md#g-1) is now concretely defined; ROADMAP gains a
  step to scaffold `ArduboyLx`.

## D-Q3: Support ArduboyG by hosting it on the 1bpp shim (was Q-3)

- **Question:** Should we support ArduboyG (4-color, high FPS) natively?
- **Decision:** Yes — hosted, not ported. ArduboyG owns **no direct buffer**:
  it overrides Arduboy2 methods and manipulates the 1bpp buffer to *simulate*
  extra gray levels, redrawing in phases. The shim can therefore host it on the
  existing 1bpp semantics with **no new buffer model**. In a sketch ArduboyG
  **replaces** Arduboy2, and `Sprites` becomes `SpritesU`. The 4 gray levels map
  onto master-palette slots ([D-Q10](DECISIONS.md#d-q10)), not a 2bpp path.
- **Priority (2026-09-26):** **lowest** — ArduboyG is used by only a small
  subset of games. The reference port
  ([ROADMAP step 11](./ROADMAP.md)) is parked behind every other step even
  though the decision stands. Meanwhile we can mine ArduboyG's API shape
  (method overrides, phase-based redraw) for **hints** that inform ArduboyLx,
  without committing to a full port.
- **Still open:** whether the time-domain frame-stacking can be simulated (vs.
  replaced by true gray) — [Q-15](./OPEN_QUESTIONS.md#q-15); whether a dedicated
  engine path is ultimately warranted is decided on evidence by the reference
  port ([ROADMAP step 11](./ROADMAP.md)).
- **Effect:** [ROADMAP step 11](./ROADMAP.md) becomes a reference port on a
  settled substrate, ranked lowest priority; the "research the API surface"
  task is answered in part (hosted model, replaces Arduboy2, uses SpritesU).

## D-Q4: Chrome = title strip + boxed pause + settings (was Q-4)

- **Question:** What belongs in the border / chrome around the 128x64 window?
- **Decision:** Reserve space in the chrome band for a **title**, in case we
  ever ship multiple Arduboy titles side by side or a launcher menu. Show a
  **boxed Pause indicator** over the center while paused, and possibly a
  small **settings menu** in the paused state that can control chrome etc.
- **Effect:** [I-4](./KNOWN_ISSUES.md#i-4) direction set; ROADMAP step 8 is
  scoped to title chrome, boxed pause, and settings-in-pause. A multi-title
  launcher would live in the **same ROM image** as the game
  ([D-Q16](DECISIONS.md#d-q16)).

## D-Q5: Support both Beep and ArduboyTones in ArduboyLx (was Q-5)

- **Question:** Is the current audio model faithful enough?
- **Decision:** Support both — the existing `BeepPin1/2`→Lynx square-wave
  mapping *and* an `ArduboyTones` port — inside ArduboyLx, delegating to the
  "real" libraries when compiling for the Arduboy.
- **Effect:** [ROADMAP step 10](./ROADMAP.md) becomes implementing the
  ArduboyTones path, not a question of *whether*.

## D-Q6: Frame rate target = as high as achievable, ~30 FPS practical (was Q-6)

- **Question:** Can we get the frame rate predictably to 59.9 FPS on 4 MHz?
- **Decision:** Aim for as high a frame rate as we can reach. Arduboy titles
  generally degrade gracefully when they miss a frame, though a few rely on
  tight timing. Lynx games rarely exceed ~30 FPS, so we may hit practical
  limits above that. Treat ~30 FPS as a realistic initial target and push up
  from there.
- **Effect:** [I-2](./KNOWN_ISSUES.md#i-2) acceptance updated; ROADMAP step 7
  targets ~30 FPS as the first milestone.

## D-Q7: Palette = programmed master registers, B/W default, packing codified (was Q-7)

- **Question:** What is the framebuffer's color model, and how can its colors be
  trusted across emulator cores and hardware (I-1's "GRB divergence")?
- **Decision:** The framebuffer holds **4-bit palette indices**, not direct
  colors. The 12-bit master palette (4,096 colors) is programmed through two
  register banks: **green** in the low nibble of `$FDA0+index`, and blue/red
  packed into `$FDB0+index` (aliases `BLUERED0..F`) with **blue in the upper
  nibble, red in the lower** (codified vs. docs; pinned per emulator core in
  step 5's color grid). A single palette-init routine programs the registers
  and serves the window renderer *and* Suzy sprites (D-Q9) — no
  SPRSYS/TST-hack colors.
- **Also settled:** the **default palette is black + white only** (slots 0 /
  7). Other color options — the 2-gray / 4-gray presets, fun/CGA monochromes,
  and the Option-1 rich-palette cycling — are **deferred**. The chrome (D-Q13)
  may use its **own** colors, so palette-init also reserves a handful of
  distinguishable colors for chrome/enhanced use, unused for now.
- **Verification note:** cross-core color truth (the original divergence worry)
  is now step-5 implementation work, not design — [I-1](./KNOWN_ISSUES.md#i-1)
  and ROADMAP step 5 carry it.
- **Effect:** ROADMAP step 5 programs the B/W default palette + reserved chrome
  colors and switches the renderer to **index writes**; I-1's acceptance is
  updated; NOTES.md records the packing and the preset decision.

## D-Q8: Emulator verification is sufficient for now (was Q-8)

- **Question:** What does "real hardware verified" require?
- **Decision:** Emulators are fine as the primary verification target for
  now; they do a good job of mimicking the Lynx for this project's purposes.
  Real-hardware verification becomes a maintainer-only, nice-to-have step.
- **Effect:** [ROADMAP step 12](./ROADMAP.md) is downgraded from a gate to an
  optional pass.

## D-Q9: Sprite rendering drives Suzy's hardware sprite engine (was Q-9)

- **Question:** Should sprite rendering use Suzy's hardware sprite engine?
- **Decision:** When a sketch uses the `Sprites` library, drive Suzy's native
  sprite path (SPRDISP/SPRCTL) as far as we can, while keeping the upstream
  `Sprites` API and a working portable reimplementation (`SpritesLynx.cpp`) as
  the software fallback. Suzy can blit **1bpp sprites natively**, which maps
  well onto the Arduboy's 1bpp sprite asset format — a real bonus. The color
  details (which master-palette indices a 1bpp sprite selects, transparency,
  collision reporting) need design care before committing; see
  [Q-14](./OPEN_QUESTIONS.md).
- **Tension resolved:** G-2's "engine verbatim" yields on the *renderer* — the
  `Sprites` *API* stays verbatim while the blit engine becomes Lynx-native, so
  the G-6/I-2 speed goal wins the frame budget back where sprites are used.
- **Effect:** [I-2](./KNOWN_ISSUES.md#i-2) gains Suzy offload as a remedy;
  ROADMAP step 7 gets a spike task; [D-Q14](DECISIONS.md#d-q14) records the
  Suzy↔`Sprites`-flags mapping decisions.

## D-Q10: Standardize on a palette-driven 4bpp framebuffer (was Q-10)

- **Question:** Which Lynx display modes should we drive — 4bpp now, 2bpp later?
- **Decision:** The Lynx screen framebuffer is a **palette-indexed 4bpp**
  surface; standardize on it for the game window. There is no "2bpp display
  track" in scope — 1bpp is a *sprite color-depth* option (Suzy), per D-Q9,
  and 4-gray (ArduboyG) content maps onto master-palette entries instead of a
  2bpp path.
- **Dissenting note:** MonLynx hardware docs and NOTES.md describe `DISPCTL` B2
  as selecting a 2-bit *display* mode. D-Q10 kept that out of scope for the
  native/enhanced surface — but the dissent is **now resolved**:
  [D-Q17](DECISIONS.md#d-q17) confirms the 2-bit display mode is real and makes
  it the default for unmodified B/W programs. D-Q10 continues to standardize
  the *enhanced* surface on 4bpp.
- **Effect:** [NOTES.md](./NOTES.md) documents the palette-index color model and
  the master palette registers; [I-1](./KNOWN_ISSUES.md#i-1) gains an
  "unprogrammed palette" hypothesis; ROADMAP step 11's substrate selection is
  resolved (choose 4 palette slots, not a 2bpp display path).

## D-Q11: Size ceiling = the Lynx RAM image budget (~48 KB); guard at build time (was Q-11)

- **Question:** What's the realistic size/memory ceiling for a Lynx game image?
- **Decision:** The hard ceiling is the **Lynx RAM image budget** — a 64 KB
  bank minus two 8160-byte 4bpp framebuffers = **~48 KB** for code + data +
  stack/zp ([D-Q17](DECISIONS.md#d-q17)'s 2-bit mode shrinks the buffers to
  ~4080 B each, raising that to ~56 KB). A stock Arduboy sketch occupies
  ≤ 32 KB flash, so it fits comfortably — the guard matters for sketches that
  would have ridden the FX chip. Plan: measure at build time how much a sketch
  places in constant / FX-style storage and **fail the build** with a clear
  message rather than emit an image that needs resources the Lynx doesn't have.
- **Resolved by [D-Q16](DECISIONS.md#d-q16):** FX-backed assets are handled by
  squashing everything into the RAM image by default (size-analysis guard),
  with FX-API support deferred. The launcher idea in D-Q4 shares the same ROM.
- **Effect:** [ROADMAP step 2](./ROADMAP.md) gains a size/FX guard task;
  [KNOWN_ISSUES.md](./KNOWN_ISSUES.md) gains I-8 (resolved by D-Q16); ceiling
  arithmetic recorded in [NOTES.md](./NOTES.md).

## D-Q12: ArduboyLx exposes the native 4bpp framebuffer through methods (was Q-12)

- **Question:** Should ArduboyLx expose a native framebuffer, or mirror the 1bpp API?
- **Decision:** Yes — ArduboyLx can expose the Lynx framebuffer(s) directly to
  Lynx-first games, with two stated caveats: the surface is **palette-indexed
  4bpp**, and it is **double-buffered**. ArduboyLx should own the VBL swap (as
  `Arduboy2Core` already does) so games never manage buffers, and expose the
  buffer behind a **method** rather than a public variable, keeping the swap
  implementation free to change. The 1bpp drop-in path stays intact for stock
  sketches (G-1); how the two coexist is
  [D-Q18](DECISIONS.md#d-q18) (the accessor is a **pointer to the back
  buffer**, with `display()` performing the swap automatically).
- **Also settled (Q-2 followup):** the "as-is source compiles directly" target
  covers both classic `Arduboy` and `Arduboy2` API families.
- **Effect:** [G-1](./GOALS.md#g-1)'s ArduboyLx surface is concretized; the
  scaffold step lands as [ROADMAP step 9](./ROADMAP.md) (steps 9–11 renumber
  to 10–12); [D-Q18](DECISIONS.md#d-q18) tracks native/1bpp coexistence.

## D-Q13: Chrome is hybrid — framework-drawn, game-tweakable (was Q-13)

- **Question:** Who draws the chrome, and at what cost?
- **Decision:** Hybrid. The framework draws the chrome (title strip, boxed
  pause, settings) by default and, when possible, only re-inks it when it
  changes — stamped once into a separate plane instead of per pixel every
  frame. A couple of small methods let the game tweak chrome content (title
  area for HP, a location name, etc.) without taking ownership of it.
- **Effect:** [ROADMAP step 8](./ROADMAP.md) is unblocked and implements the
  hybrid model; [I-4](./KNOWN_ISSUES.md#i-4)'s ownership question is closed.

## D-Q14: Suzy sprite path settled; build-time pre-transpose is the favored approach (was Q-14)

- **Question:** How do Suzy's native sprite features map onto the upstream
  `Sprites` API?
- **Decision:** Drive Suzy's SPRDISP/SPRCTL path for `Sprites`, keeping
  `SpritesLynx.cpp` as the software fallback (D-Q9); upstream flags
  (`H_FLIP`, `V_FLIP`, `CLEAR`, `ALPHA`) and draw modes
  (`PS_MASKED`, `PS_OR`, `PS_AND`, `PS_XOR`, ...) stay faithful.
  **1bpp sprite ink defaults to the palette of the Arduboy screen area**
  (per-sprite palette-selection enhancement hooks are future ArduboyLx work —
  parked in [Q-19](./OPEN_QUESTIONS.md#q-19)'s scope). Suzy blits **always write
  to the back buffer**, consistent with D-Q18. Verification is **eyeball /
  screenshots only** — a dedicated Suzy-vs-`SpritesLynx.cpp` golden-frame
  harness is overkill; the Step 4 paint golden and the Step 1/7 FPS bench still
  guard the pipeline.
- **Implementation path (not blocking):** static sprite arrays are
  **pre-transposed to Lynx scanline format at build time** (a `constexpr`
  `LynxSprite` wrapper / macro-injected declaration) so Suzy blits straight
  from ROM with zero runtime transpose; 8px-band stitching covers >8px-tall
  sprites; dynamic sprites transpose into a RAM scratchpad. Mechanics tracked
  in ROADMAP step 7 (d) and NOTES.md.
- **Effect:** [ROADMAP step 7 (d)](./ROADMAP.md)'s spike proceeds on decided
  inputs; Q-14 closes.

## D-Q16: FX-chip assets = squash into the RAM image; guard at build time (was Q-16)

- **Question:** What replaces the Arduboy FX chip's asset flash on the Lynx?
- **Decision:** There is **no FX replacement** on the Lynx. Default answer:
  squash everything (code + PROGMEM + any would-be-FX data) into the
  **~48 KB RAM image** — a stock sketch is ≤ 32 KB and fits, and a per-game
  build flag is the escape hatch for anything bigger. Cartridge-ROM /
  bankswitched reads of large assets are **out of scope for non-modified games
  for now**. Detection is **build-time size analysis only**; FX-API support is
  deferred, so referencing the FX APIs simply makes the build fail (D-Q11).
- **Also settled (D-Q4 interplay):** the launcher / multi-title idea lives in
  the **same Lynx ROM image** as the game.
- **Effect:** closes [I-8](./KNOWN_ISSUES.md#i-8) and resolves D-Q11's open
  thread; [ROADMAP step 2](./ROADMAP.md)'s guard is scoped to size analysis.

## D-Q17: DISPCTL 2-bit display mode is real — default for unmodified programs (was Q-17)

- **Question:** Is DISPCTL's 2-bit (4-color) display mode real, and is it ever
  worth using?
- **Decision:** **Accepted as real** (assume `DISPCTL = 0x05` shows 2-bit
  pixels: 40-byte scanlines, ~4080-byte framebuffer; double-check on first
  use — ROADMAP step 5). It shrinks the buffers (~8 KB total vs 16.3 KB at
  4bpp), freeing RAM for program/graphics and making byte translation quicker,
  so use it as the **default for unmodified (B/W) programs**. 4bpp stays the
  native/enhanced surface (D-Q10). Implementation variants are picked by
  **build flags** (templates/constexpr), not a fixed dual path.
- **Resolves:** the D-Q10 dissenting note.
- **Implementation (2026-09-26, from what-was-Q-20):** 2bpp joins
  `-DARDUBOYLX_PAINTPATH` as its **own value** (the array math is entirely
  different), and the one-shot expansion **and** the zero-transpose path each
  need a **2bpp variant** or pixels land in the wrong slots. **No palette
  cycling in 2-bit** — enhanced (ArduboyLx) content likely disables 2-bit and
  runs 4bpp anyway. Caveat: 2-bit mode is rarely/never used, so **emulators may
  not support it properly** — verify on target cores and treat failures as a
  possible emulator gap.
- **Effect:** [I-2](./KNOWN_ISSUES.md#i-2) gains a 2-bit headroom remedy (less
  expansion work per frame); NOTES.md's DISPCTL note updates; ROADMAP step 5
  verifies the mode when first tried (with the emulator-support caveat);
  [D-Q20](DECISIONS.md#d-q20) records the pipeline interplay details.

## D-Q18: Native/1bpp coexistence = back-buffer pointer + build-flag paint paths (was Q-18)

- **Question:** How do the native 4bpp and 1bpp drop-in paths coexist inside one
  ArduboyLx?
- **Decision:** The native surface is a **pointer to the back framebuffer**,
  and `display()` performs the swap automatically. A stock game's single
  `sBuffer` is the back buffer, so read-back keeps working (the displayed front
  buffer may differ — which makes the screen-sized-Suzy-sprite scratchpad
  experiment more appealing). **Mixing is supported**: direct canvas writes and
  native sprites both target the back buffer (native sprite draws may come out
  of order vs. canvas draws — monitor). `clear()` / `fillScreen()` map to the
  same operations on the Lynx buffer. The 1bpp→4bpp expansion's behavior will
  eventually be chosen by the `ARDUBOYLX_PAINTPATH` build flag; **until then,
  keep copying 1bpp data into the Lynx buffer.**
- **Priority:** full support of every Arduboy draw method writing directly to
  the Lynx buffer **first**; sBuffer/getBuffer() fan-out solutions come after.
- **Deferred (low priority):** exposing both buffers and/or explicit flipping
  for enhanced games — see [Q-19](./OPEN_QUESTIONS.md#q-19).
- **Effect:** concretizes D-Q12's accessor; ROADMAP step 9 shapes the
  `fbBuffer()`/`display()` contract; [Q-19](./OPEN_QUESTIONS.md#q-19) tracks the
  deferred enhanced-surface features.

## D-Q20: 2-bit mode plumbing — new paint-path value, per-path variants, no cycling (was Q-20)

- **Question:** How does the 2-bit display mode (D-Q17) thread through the
  pipeline — flags, geometry, palette, verification?
- **Decision:**
  - **Flag habitation:** 2bpp is a **new `ARDUBOYLX_PAINTPATH` value** — the
    array math is entirely different (40-byte scanlines, 4×2-bit pixels per
    byte).
  - **Geometry:** the one-shot expansion **and** the zero-transpose path each
    need a **2bpp variant**, or pixels land in the wrong place (the 16/19
    centering and 128×64 clip are unchanged; the byte arithmetic is not).
  - **Chrome + palette:** **no palette cycling in 2-bit**; content built on
    ArduboyLx enhancements likely disables the 2-bit mode and runs 4bpp.
  - **Verification caveat:** 2-bit (4-color) mode is rarely or never used, so
    **emulators may not support it properly** — confirm on first use and treat
    weird output as a possible emulator gap (ROADMAP step 5).
- **Effect:** ROADMAP step 5's 2-bit task checks emulator support; step 6's
  paint-path flag set `{one-shot | native | measure}` gains `2bit`;
  implementation notes folded into
  [D-Q17](DECISIONS.md#d-q17); Q-20 closes.