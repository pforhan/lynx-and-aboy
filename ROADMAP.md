# Roadmap

Discrete, testable steps for the `lynx-and-aboy` port. Each step has a goal,
the `G-` / `Q-` / `I-` items it serves, prerequisites, concrete tasks, and
**exit criteria** — a command and a pass condition so "done" is unambiguous.

Status legend: `[ ]` not started · `[~]` in progress · `[x]` done.

Objects referenced: **G** = [GOALS.md](./GOALS.md), **Q** =
[OPEN_QUESTIONS.md](./OPEN_QUESTIONS.md), **I** =
[KNOWN_ISSUES.md](./KNOWN_ISSUES.md), **D** =
[DECISIONS.md](./DECISIONS.md) (settled questions).

Settled design decisions (D-Q*) are *inputs* to steps here; open questions
(Q-*) are *blockers* or topics a step must resolve.

---

## Phase 0 — Baseline (know where we stand)

### 1. Re-measure and record the current state `[ ]`

Serves: undocumented — prerequisite for everything else.

The two HIGH issues (color, 5 FPS) are only eyeballed. Before changing
anything, capture a reliable baseline so fixes have a before/after.

Tasks:

- [ ] Add a repeatable FPS measurement via the **`measure` build option**
      (`-DARDUBOYLX_PAINTPATH=measure`, one-shot renderer + frame-timing
      harness — see `NOTES.md`), cross-checked against an emulator-based
      frame-count over wall-clock (Mednafen framebuffer dumps). Note the
      environment (emulator, core, host CPU, `-O` flags) in `NOTES.md`.
- [ ] Capture before-fix screenshots: Mednafen `F9` exact-framebuffer PNG for
      boot logo, demo, inverted mode, rich palette, and Pause overlay.
- [ ] Record the measured FPS and a description of exactly what "green" looks
      like (which glyphs/areas are green, is anything non-green?).

Exit criteria:

- [ ] FPS is measured over ≥100 frames by a repeatable procedure, and the
      number is recorded in `NOTES.md`.
- [ ] At least the demo + rich-palette screenshots exist in the repo (or a
      linked store) before any fix is applied.

---

## Phase 1 — Hardening (make regressions loud)

### 2. Assert the BLL payload entry point and image budget at build time `[ ]`

Serves: I-7, D-Q11 · Prereq: Step 1 not required.

The zero-page/BLL layout trap (`payload[0]` must be `_start`, not data) is
silently easy to regress, and an oversized or FX-backed image is a silent
"runs on Arduboy, not on Lynx" trap of its own
([D-Q11](./DECISIONS.md#d-q11)). Make the build fail loudly on both.

Tasks:

- [ ] Add a `build-lynx.sh` post-link check that disassembles/reads
      `payload[0]` and asserts it equals `_start` (= `a9 00 85 00` prologue
      or the linker symbol), else print the "INSERT GAME" hazard and exit 1.
- [ ] Size/FX guard per D-Q11: `build-lynx.sh` reports the emitted image size
      and fails if static/FX-style assets exceed the RAM image budget or the
      sketch references FX-chip APIs (the Lynx has no FX flash). Long-term
      substitute: squash everything into the ~48 KB RAM image by default
      ([D-Q16](./DECISIONS.md#d-q16)).
- [ ] Add a negative test: temporarily build with the *stock* platform link
      script and confirm the check fails; revert.

Exit criteria:

- [ ] A deliberately-broken layout fails `build-lynx.sh` with a clear message.
- [ ] A sketch that exceeds the RAM/storage budget fails the build with a
      clear "won't fit on Lynx" message.
- [ ] Clean build passes and the image still loads; test covered in Step 4's
      harness.

### 3. Toolchain pre-flight check + help text `[ ]`

Serves: G-6.

A newcomer should get a clear, immediate explanation when the toolchain is
missing or wrong.

Tasks:

- [ ] Add a `check-toolchain` script (or `build-lynx.sh --check`) that
      verifies: compiler present, expected version, `LYNX_TOOLCHAIN`/
      `PATH` config, and the platform link script path — each with a plain-
      language fix hint.
- [ ] Add `--help` output to `build-lynx.sh` and `build-arduboy.sh` covering
      usage, env vars, and example invocation.

Exit criteria:

- [ ] `./build/check` on a machine *without* the toolchain prints one obvious
      "what to install and where" message and exits non-zero.
- [ ] `build-lynx.sh --help` renders usage without touching the toolchain.

---

## Phase 2 — Host test harness (verify without a Lynx)

### 4. Deterministic host tests `[ ]`

Serves: I-3 · Prereq: Step 1 (baseline screenshots) and Step 2 are useful
context.

A build is not "done" unless the non-hardware claims are verified on the host
compiler.

Tasks:

- [ ] Structure: a host-test directory (`test/`) with runnable unit tests.
      Decide a build runner (`make test`, a shell script, or a tiny C++ test
      main) and document the command in `CONTRIBUTING.md`.
- [ ] **Clock test:** run `lynx_micros()`/`lynx_millis()` against a mocked
      TIM4 register to assert monotonicity and the 256µs-fold math.
- [ ] **Button test:** feed synthetic keypad bytes (`$FCB0`/`$FCB1`/
      `$FC92`) and assert the Arduboy button byte from `buttonsState()` maps
      correctly for both orientations.
- [ ] **Golden-frame test** (after Step 5 fixes color): render a fixed
      sBuffer with `paintScreen()` into a mocked framebuffer and diff against
      a checked-in golden binary. Regenerate goldens deliberately, never by
      accident.

Exit criteria:

- [ ] `make test` (or documented equivalent) passes on a clean checkout with
      zero toolchain dependency.
- [ ] A deliberately-introduced byte flip in the golden binary fails the test.

### 5. Diagnose and fix the color bug `[ ]`

Serves: I-1, D-Q7.

*High priority — this is the visible symptom blocking the "playable" claim.*

The [D-Q7](./DECISIONS.md#d-q7) palette answer corrects the model:
the framebuffer holds **4-bit palette indices**, not direct GRB colors, and
colors come from the master palette (`$FDA0-$FDBF`). The renderer writes raw
GRB values (`0x07`, `richPal[]`) without programming the palette — strong new
hypothesis 0 for "everything green".

Tasks:

- [ ] **Palette hypothesis 0 (new):** check whether the default master palette
      maps the indices the renderer already writes (`0x07`, `richPal[]`) to the
      intended colors. Program the default **B/W** palette (slots 0 = black,
      7 = white — D-Q7; other color options deferred) in
      `Arduboy2Core::boot()` and see if white turns white. Also reserve a few
      distinguishable colors for the chrome (D-Q13), unused for now.
- [ ] **2-bit display mode (D-Q17/D-Q20):** verify `DISPCTL = 0x05` shows real
      2-bit pixels (40-byte scanlines, ~4080-byte framebuffer) and confirm the
      RAM headroom / faster byte translation. Caveat: 2-bit mode is rarely
      used, so **emulators may not support it properly** — treat weird output
      as a possible emulator gap (D-Q20). If it works, wire it as the default
      for unmodified B/W sketches behind the `2bit` paint-path value (see
      [D-Q20](./DECISIONS.md#d-q20)).
- [ ] Cross-check the green symptom on a second emulator (Handy is a good
      second core; BLL loader under Handy if needed) to rule hypothesis 1 of
      I-1 (emulator interpretation).
- [ ] Verify `DISPCTL` really reads `0x0D` (color + 4-bit + DMA) at runtime;
      rule out hypothesis 2.
- [ ] Write a 2-emulator + hardware grid: render a 16-color test pattern that
      **programs the master palette** (each of the 16 slots = a distinct
      GRB shade), snapshot on each, and compare against the programmed values.
      D-Q7 codifies the expected packing (**blue upper nibble, red lower**, in
      `$FDB0+index`; green low nibble in `$FDA0+index`) — the grid confirms it
      per core.
- [ ] Apply the smallest change that makes white = white in the target
      renderer(s); switch `INK_MONO`/`richPal` to palette-**index** writes and
      add one palette-init routine that both `paintScreen()` and the Suzy
      sprite path (D-Q9) share.

Exit criteria:

- [ ] `F9` framebuffer png shows pure white pixels as white on ≥2
      emulator cores, and the 16-color pattern matches the programmed palette.
- [ ] Default palette = B/W (D-Q7); rich-palette (`Option 1`) cycling is
      deferred, so no distinct-columns requirement until color options return.
- [ ] Golden-frame test (Step 4) updated to encode the corrected output.

---

## Phase 3 — Fit for real games

### 6. Survey real Arduboy games for `sBuffer` direct access `[ ]`

Serves: Q-2 · informs G-1 and Step 7.

Tasks:

- [x] Pick 8-10 popular open-source Arduboy games (e.g. Arduboy collection,
      "Tracy+", game jam winners).
- [x] For each: does it use `sBuffer` directly, `Sprites`, `display()`
      frequency, and any third-party libs? Note which Arduboy API version
      each targets. Classified 2026-09-26 (8 games) — see the table in
      `NOTES.md`.
- [ ] Build the ones that use only the supported surface unmodified for the
      Lynx; record which fail and why.
- [ ] Q-2's detection followup is **deferred**: D-Q18 orders "methods first,
      sBuffer/getBuffer() later", so pick the detection mechanism (LTO static
      analysis / `#define` accessor / runtime marker) only after the method
      layer is complete.
- [ ] Add a paint-path build flag (e.g. `-DARDUBOYLX_PAINTPATH={one-shot|native|measure|2bit}`)
      selecting between the native-1bpp + one-shot convert, the zero-transpose
      native-write path (D-Q18), the measuring-harness build
      (`measure` = one-shot renderer + frame-timing instrumentation, which also
      serves Step 1's FPS baseline and Step 7's regression benchmark), and the
      **2bpp display mode** (a distinct value — the array math is entirely
      different, [D-Q20](./DECISIONS.md#d-q20); each of the one-shot and
      zero-transpose paths needs its own 2bpp variant).
      **Default: `one-shot`** for now (predictable, matches heavy users); flip
      per game when the survey/flag or the crossover says native wins. Record
      the criterion in `NOTES.md`.
- [x] Classify survey games by API family (classic `Arduboy` vs `Arduboy2`);
      treat the breakout as informational — the Q-2 answer is that as-is
      source for *both* families must compile directly (see D-Q12, recorded in
      NOTES.md).

Exit criteria:

- [ ] A classification table exists in `NOTES.md` with a per-game "builds
      unmodified?" column.
- [ ] A concrete answer to "what must work for a drop-in port" is recorded,
      feeding the G-1 acceptance test (see
      [D-Q1](./DECISIONS.md#d-q1-ship-an-arduboylx-api-layer-was-q-1)).
- [ ] Q-2's detection followup is explicitly deferred (D-Q18 "methods first")
      or decided outright by the time the method layer lands.

### 7. Raise the frame rate to the D-Q6 target `[ ]`

Serves: I-2 · target set by D-Q6. Prereq: Steps 1 (baseline FPS) and 6
(know the real workload) inform the work. Step 5 makes the visuals trustworthy
enough to see the result of optimization.

Target: **~30 FPS as the first milestone, pushing as high as possible**
([D-Q6](./DECISIONS.md)); Lynx games and the demo's `setFrameRate(30)` both
suggest 30 is a realistic floor to beat.

Tasks:

- [ ] Establish the Step 1 measurement as the regression benchmark.
- [ ] Profile the hot path: instrument `paintScreen()` inner loop cost; try
      (a) precomputed per-row nibble lookup table eliminating the
      `alphaPixel()` call, (b) hoisting the border to a one-time scratch
      "chrome" buffer combined at swap (see Step 8), (c) stamping the
      border/chrome once and only re-inking it when it changes per
      [D-Q13](./DECISIONS.md#d-q13), (d) a spike driving `Sprites` through
      Suzy's SPRDISP/SPRCTL path per
      [D-Q9](./DECISIONS.md#d-q9) to cut per-sprite CPU cost (fall back to
      `SpritesLynx.cpp` otherwise) — pre-transposing static sprite arrays to
      Suzy scanline format at build time (constexpr wrapper) removes their
      transpose from the frame budget entirely; see NOTES.md. **Verification
      is eyeball/screenshots — no dedicated Suzy-vs-software golden harness
      (D-Q14);** the Step 4 paint golden and the FPS bench still guard the
      pipeline.
- [ ] **Full-screen Suzy blit experiment** (see NOTES.md): transpose the live
      sBuffer to a 1bpp sprite scratch (~1.1 KB) with an 8×8-LUT bit gather,
      blit the whole 128×64 screen through Suzy, and diff the measured FPS
      against the direct nibble path. Record the delta; keep the faster path.
      Known limits: B/W only (rich Option-1 palette breaks), chrome must be
      drawn separately.
- [ ] **Zero-transpose paint path experiment** (D-Q18): reimplement the draw
      API to write 4bpp-native nibbles into the back buffer (compile-time
      folded offset/clip, `display()` = DISPADR swap) and diff FPS against the
      direct nibble path and the Suzy-blit path. API-only games are the win;
      direct-`sBuffer` pokes fall back to a proxy shim or the 1bpp path. See
      NOTES.md.
- [ ] Re-measure after each change and record the delta in `NOTES.md`. Keep
      the change that meets the D-Q6 target; keep others documented for
      reference.

Exit criteria:

- [ ] FPS at/above the ~30 FPS D-Q6 target for ≥100 frames via Step 1's
      method, on the emulator we support, with zero visible tearing/flicker.
- [ ] `paintScreen()` correctness unchanged — Step 4 golden-frame test still
      green.

---

## Phase 4 — Lynx-native polish

### 8. Implement the hybrid border/chrome per D-Q4 + D-Q13 `[ ]`

Serves: I-4 · direction set by D-Q4, ownership settled by
[D-Q13](./DECISIONS.md#d-q13) (hybrid) · Prereq: Step 7 (chrome must not hurt
FPS).

The *what* is decided ([D-Q4](./DECISIONS.md)): title strip space, boxed Pause
indicator over the center, and possibly a pause-time settings menu. The *who*
and *how cheaply* are also decided ([D-Q13](./DECISIONS.md)): the framework
draws the chrome, re-inking only on change, and exposes small game tweak
methods.

Tasks:

- [ ] Implement the hybrid model per D-Q13: the framework draws the chrome
      (title strip, boxed pause, settings) by default, stamping it once and
      re-inking only when it changes; record the model in `NOTES.md`.
- [ ] Expose the small game-tweak methods D-Q13 calls for (e.g. title area for
      HP, location name) behind the same API.
- [ ] On Pause, show the boxed pause indicator over the center of the game
      window (per D-Q4), and surface system hints ("Option 1 = palette")
      in the title strip.
- [ ] If the pause-time settings menu is in scope, spec and implement it
      (chrome on/off, title display, etc.).

Exit criteria:

- [ ] The hybrid chrome is visible and correctly rendered on ≥2 emulator
      cores (screenshot in repo).
- [ ] FPS regression vs Step 7 baseline ≤ the Step 7 measurement tolerance.
- [ ] I-4 is resolved (border decision recorded; D-Q13 ownership closes the
      open question).

### 9. Scaffold the ArduboyLx API layer `[ ]`

Serves: D-Q1, D-Q12 · Prereq: Steps 4 and 5 (host-test harness + trustworthy
colors) are useful context.

[D-Q1](./DECISIONS.md#d-q1-ship-an-arduboylx-api-layer-was-q-1) promises an
`ArduboyLx` API that falls back to standard Arduboy semantics on the Arduboy;
[D-Q12](./DECISIONS.md#d-q12) shapes its Lynx side: expose the native 4bpp
palette-indexed framebuffer through **methods** (not a public variable), with
ArduboyLx owning the double-buffer swap. Stock 1bpp sketches keep byte-exact
semantics (G-1).

Tasks:

- [ ] Define the ArduboyLx surface: `fbBuffer()` returns a pointer to the
      **back** framebuffer and `display()` flips automatically
      ([D-Q18](./DECISIONS.md#d-q18) — also `fbSetPixel(x, y, index)`,
      `fbGetPixel(...)`), plus how native and 1bpp draws coexist in one frame
      (D-Q18).
- [ ] Add `#if defined(__LYNX__)` native path + Arduboy-side fallbacks so the
      same header builds on both backends (D-Q1).
- [ ] Write a minimal `ArduboyLx.h` scaffold that compiles on both targets and
      document the API contract in `NOTES.md`.
- [ ] Fold the Q-2 "as-is source compiles directly" acceptance (both classic
      `Arduboy` and `Arduboy2`) into the G-1 test.

Exit criteria:

- [ ] A new `examples/lynx-native` sketch compiles for both Lynx and Arduboy and
      draws through the native surface while the demo still owns the swap.
- [ ] The existing 1bpp demo still builds unmodified (drop-in mode intact).
- [ ] D-Q12's "methods, not variables" interface is documented.

---

## Phase 5 — Ecosystem

### 10. Implement both audio paths (Beep + ArduboyTones) `[ ]`

Serves: D-Q5 · Prereq: Steps 4 and 7 (need a testable audio path and a
working frame budget).

[D-Q5](./DECISIONS.md) settles the *what*: ArduboyLx supports both the Beep
mapping and an ArduboyTones port, delegating to the "real" libraries when
compiling for the Arduboy.

Tasks:

- [ ] Port MLXXXp's `ArduboyTones` to the Lynx backend (map its two tone
      channels onto the existing square-wave code).
- [ ] Add a tone-stack host test (sequences play in the right order/timing)
      using the Step 4 harness.
- [ ] Build the upstream ArduboyTones demo sketch unmodified for the Lynx.
- [ ] Define the ArduboyLx audio API shape so both paths live behind one
      interface (Beep-based vs Tones-based games pick transparently).

Exit criteria:

- [ ] The ArduboyTones demo sketch builds unmodified and produces audible,
      correctly-sequenced tones on the emulator (and hardware if available).
- [ ] Host test for the tone stack is green.
- [ ] Beep-based sketches from Step 6 still sound unchanged.

### 11. ArduboyG-style 4-color reference port `[ ]` — LOWEST priority

Serves: D-Q3 · Prereq: Steps 7 and 10 (frame budget + audio surface).

**Lowest priority:** ArduboyG is used by only a small subset of games
([D-Q3](./DECISIONS.md#d-q3), 2026-09-26). The design is settled — this step
is parked behind *all* other work, including Step 12 (hardware) — but the
decision stands and ArduboyG's API shape (method overrides, phase-based
redraw) is worth mining for **hints** while building ArduboyLx even before
this port is attempted.

[D-Q3](./DECISIONS.md#d-q3) settles the *what*: host ArduboyG on the 1bpp shim
(it overrides Arduboy2 methods, manipulates the 1bpp buffer to simulate gray,
and replaces `Sprites` with `SpritesU`). This step builds the reference port to
confirm it on evidence and decide the gray-level mapping. The substrate is
already fixed by [D-Q10](./DECISIONS.md#d-q10) (4 master-palette slots on the
4bpp frame) — see [Q-15](./OPEN_QUESTIONS.md#q-15) for the gray-level mapping.

Tasks:

- [ ] Confirm the D-Q3 hosted model against upstream ArduboyG (it replaces
      Arduboy2, uses `SpritesU`; no coexistence question remains).
- [ ] Map the 4 gray levels onto 4 master-palette slots (slots per Q-15);
      record in `NOTES.md`.
- [ ] Port the ArduboyG pattern as `examples/lynx-g` (4 gray levels → 4 Lynx
      colors; high frame rate preserved).
- [ ] Handle phase redraw (Q-15 answer): game logic once per frame incarnation,
      graphics every phase pass — simulate on the Lynx's continuous-DMA display.
- [ ] Measure its FPS with Step 1's method; record in `NOTES.md`.

Exit criteria:

- [ ] `examples/lynx-g` reaches the D-Q6 target FPS and renders distinct,
      correctly-colored levels (screenshot).
- [ ] D-Q3's hosted model holds up on evidence (dedicated path vs. a general
      frame-stacking API); the substrate question is closed by D-Q10.

---

## Phase 6 — Hardware truth

### 12. Real-hardware verification pass (optional, maintainer-only) `[ ]`

Serves: G-5, Q-7 · Prereq: Steps 5 and 7 (correct colors + playable frame
rate).

[D-Q8](./DECISIONS.md) makes this optional: emulators are the primary
verification target for now. Run this pass when hardware is available.

Tasks:

- [ ] Push `lynx-demo.bll.o` via the ComLynx `bll` loader onto a real unit.
- [ ] Run the full manual checklist in `NOTES.md`; fix anything that only
      shows up on silicon (colors, pause, palette, orientation).
- [ ] If any fix diverges from emulator behavior, reconcile which emulator
      core to trust and record it.

Exit criteria (when run):

- [ ] The `NOTES.md` manual checklist passes on real hardware, with a
      timestamp and unit revision recorded.
- [ ] Emulator-vs-hardware discrepancies are documented (or Q-7 resolved).

---

## Definition of done (project-wide)

The port is "seamless" per [D-Q1](./DECISIONS.md#d-q1) when all of the
following hold:

- [ ] Any Arduboy2 sketch from the Step 6 survey's "supported surface" class
      builds **unmodified** for the Lynx and is playable (classic `Arduboy`
      and `Arduboy2` alike — D-Q12).
- [ ] The demo runs at the D-Q6 target FPS with correct colors on ≥2 emulator
      cores; real hardware is a maintainer-only bonus pass
      ([D-Q8](./DECISIONS.md)).
- [ ] `make test` (Step 4) passes; build fails loudly on toolchain/layout and
      image-budget problems (Steps 2-3).
- [ ] `Sprites` sketches run through Suzy with the software fallback intact
      ([D-Q9](./DECISIONS.md)), and the native 4bpp surface exists
      ([D-Q12](./DECISIONS.md), [D-Q18](./DECISIONS.md), step 9).
- [ ] Unmodified B/W programs run in 2-bit display mode by default
      ([D-Q17](./DECISIONS.md)) once verified in step 5.
- [ ] Oversized / FX-backed builds fail at compile time with a clear message
      ([D-Q11](./DECISIONS.md), [D-Q16](./DECISIONS.md)); ArduboyG-style
      sketches host on the 1bpp shim ([D-Q3](./DECISIONS.md), step 11 —
      lowest priority).
- [ ] Documentation is in sync: README quickstart, NOTES.md register truth,
      this roadmap, and the issue files.