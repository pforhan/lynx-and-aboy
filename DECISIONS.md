# Decisions

Settled design decisions, moved here from
[OPEN_QUESTIONS.md](./OPEN_QUESTIONS.md). Each keeps its original `Q-` id
(e.g. **D-Q1** was formerly `Q-1`) so the rest of the docs can reference the
decision history without ambiguity. The rationale and any dissent are recorded
here; the *effect* on code, issues, and roadmap is disseminated in the file
each decision points to.

Date recorded: 2026-09-08. Additional decisions recorded 2026-09-23
(D-Q9, D-Q10, D-Q11, D-Q12, D-Q13).

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

## D-Q4: Chrome = title strip + boxed pause + settings (was Q-4)

- **Question:** What belongs in the border / chrome around the 128x64 window?
- **Decision:** Reserve space in the chrome band for a **title**, in case we
  ever ship multiple Arduboy titles side by side or a launcher menu. Show a
  **boxed Pause indicator** over the center while paused, and possibly a
  small **settings menu** in the paused state that can control chrome etc.
- **Effect:** [I-4](./KNOWN_ISSUES.md#i-4) direction set; ROADMAP step 8 is
  scoped to title chrome, boxed pause, and settings-in-pause.

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
  ROADMAP step 7 gets a spike task; [Q-14](./OPEN_QUESTIONS.md#q-14) tracks the
  Suzy↔`Sprites`-flags mapping.

## D-Q10: Standardize on a palette-driven 4bpp framebuffer (was Q-10)

- **Question:** Which Lynx display modes should we drive — 4bpp now, 2bpp later?
- **Decision:** The Lynx screen framebuffer is a **palette-indexed 4bpp**
  surface; standardize on it for the game window. There is no "2bpp display
  track" in scope — 1bpp is a *sprite color-depth* option (Suzy), per D-Q9,
  and 4-gray (ArduboyG) content maps onto master-palette entries instead of a
  2bpp path.
- **Dissenting note:** MonLynx hardware docs and NOTES.md describe `DISPCTL` B2
  as selecting a 2-bit *display* mode. D-Q10 keeps that out of scope for now and
  pins verification to [Q-17](./OPEN_QUESTIONS.md#q-17).
- **Effect:** [NOTES.md](./NOTES.md) documents the palette-index color model and
  the master palette registers; [I-1](./KNOWN_ISSUES.md#i-1) gains an
  "unprogrammed palette" hypothesis; ROADMAP step 11's substrate selection is
  resolved (choose 4 palette slots, not a 2bpp display path).

## D-Q11: Size ceiling = the Arduboy 32 KB constraint; guard at build time (was Q-11)

- **Question:** What's the realistic size/memory ceiling for a Lynx game image?
- **Decision:** A stock Arduboy sketch's code and assets must already fit the
  Arduboy's 32 KB flash — anything more rides the optional FX chip. The Lynx RAM
  image (64 KB bank minus two 8160-byte framebuffers and the program) is not the
  practical binding constraint. Plan: measure at build time how much a sketch
  places in constant / FX-style storage and **fail the build** with a clear
  message rather than emit an image that needs resources the Lynx doesn't have.
- **Still open:** what, if anything, replaces FX-chip asset flash
  (cartridge-ROM images, bankswitching) — see
  [Q-16](./OPEN_QUESTIONS.md#q-16). Also affects the launcher idea in D-Q4.
- **Effect:** [ROADMAP step 2](./ROADMAP.md) gains a size/FX guard task;
  [KNOWN_ISSUES.md](./KNOWN_ISSUES.md) gains I-8; ceiling arithmetic recorded in
  [NOTES.md](./NOTES.md).

## D-Q12: ArduboyLx exposes the native 4bpp framebuffer through methods (was Q-12)

- **Question:** Should ArduboyLx expose a native framebuffer, or mirror the 1bpp API?
- **Decision:** Yes — ArduboyLx can expose the Lynx framebuffer(s) directly to
  Lynx-first games, with two stated caveats: the surface is **palette-indexed
  4bpp**, and it is **double-buffered**. ArduboyLx should own the VBL swap (as
  `Arduboy2Core` already does) so games never manage buffers, and expose the
  buffer behind a **method** rather than a public variable, keeping the swap
  implementation free to change. The 1bpp drop-in path stays intact for stock
  sketches (G-1); how the two coexist is [Q-18](./OPEN_QUESTIONS.md#q-18).
- **Also settled (Q-2 followup):** the "as-is source compiles directly" target
  covers both classic `Arduboy` and `Arduboy2` API families.
- **Effect:** [G-1](./GOALS.md#g-1)'s ArduboyLx surface is concretized; the
  scaffold step lands as [ROADMAP step 9](./ROADMAP.md) (steps 9–11 renumber
  to 10–12); [Q-18](./OPEN_QUESTIONS.md#q-18) tracks native/1bpp coexistence.

## D-Q13: Chrome is hybrid — framework-drawn, game-tweakable (was Q-13)

- **Question:** Who draws the chrome, and at what cost?
- **Decision:** Hybrid. The framework draws the chrome (title strip, boxed
  pause, settings) by default and, when possible, only re-inks it when it
  changes — stamped once into a separate plane instead of per pixel every
  frame. A couple of small methods let the game tweak chrome content (title
  area for HP, a location name, etc.) without taking ownership of it.
- **Effect:** [ROADMAP step 8](./ROADMAP.md) is unblocked and implements the
  hybrid model; [I-4](./KNOWN_ISSUES.md#i-4)'s ownership question is closed.