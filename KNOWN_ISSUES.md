# Known Issues

Definite bugs, gaps, and hazards. Each has an ID (`I-`), a severity, the
evidence so far, and a passing acceptance criterion. Progress is tracked in
[ROADMAP.md](./ROADMAP.md).

## I-1 (HIGH): Everything renders green

- *Symptom:* the demo draws full-white `0x07` pixels, but the
  panel/emulator shows everything green (or shades of green), even though
  `INK_MONO` is `0x07` (bits 0-2 set = white in GRB).
- *Notes:* the code paths themselves look internally consistent (even-x =
  high nibble, odd-x = low nibble in both `paintScreen()` and
  `paint8Pixels()`).
- *Model update (2026-09-23):* the framebuffer stores **4-bit palette
  indices**, not direct GRB colors — colors come from the master palette at
  `$FDA0-$FDBF` ([Q-7 answer](OPEN_QUESTIONS.md#q-7),
  [NOTES.md](./NOTES.md)). The renderer writes raw GRB values (`0x07`,
  `richPal[]`) without programming the palette, which only looks right if the
  default palette maps those indices to the intended colors.
- *Hypotheses to rule out, in order:*
  0. **Unprogrammed palette** (new, now most likely) — default master-palette
     slot 7 isn't white (or the rich slots aren't rich), because nothing in
     `boot()` programs `$FDA0-$FDBF`. Fix: palette-init in `boot()`, renderer
     switches to index writes (ROADMAP step 5).
  1. **Emulator interpretation** — the Beetle Lynx core may render the 4bpp
     palette differently than docs (needs a cross-check against a second
     emulator and, ideally, one known-good hardware screenshot).
  2. **DISPCTL mode** — verify we're actually in 4-bit mode (`DISPCTL =
     0x0D`) and not 2-bit (`0x05`), where the color interpretation collapses
     and only some nibbles survive.
  3. **Nibble/packing endianness on the panel** — even-x vs odd-x high/low
     nibble assignment may be inverted on real silicon vs. the emulator we
     developed against.
  4. **Color enable bit** — double-check `DISPCTL_COLOR` vs. mono mode; if
     we ended up in mono 2-bit, the GRB encoding isn't applied at all.
- *Acceptance:* after any fix, the manual color checklist in
  [NOTES.md](./NOTES.md) passes on at least two emulator cores, the master
  palette is programmed so index 7 = white, with `mednafen`'s exact-framebuffer
  snapshot (`F9`) confirming pure-white is truly white.

## I-2 (HIGH): ~5 FPS in the Lynx emulator, target ~30

- *Symptom:* ~5 FPS measured in Mednafen 1.29.0 / Beetle Lynx (treat as an
  indication, not a spec, until re-measured — see ROADMAP step 1).
- *Likely contributors, in priority order:*
  1. Per-pixel 1bpp→4bpp expansion in software (`paintScreen()`, ~4096
     pixel-pair bytes/frame); no 65C02 SIMD.
  2. Per-frame `drawWindowBorder()` (extra 128+64 byte ops).
  3. Busy-polling the frame/clock (CPU never idle).
  4. Emulator overhead compounding (Beetle Lynx core).
- *Ideas:* profile the hot loop; precompute nibble pairs per row to avoid
  the `alphaPixel`/palette function call per pixel; defer the border to a
  single setup pass rather than every paint ([D-Q13](./DECISIONS.md)); offload
  sprite blits to Suzy's SPRDISP path ([D-Q9](./DECISIONS.md)).
- *Target:* [D-Q6](./DECISIONS.md) sets **~30 FPS as the realistic first
  milestone**, pushing as high as we can over time.
- *Acceptance:* the demo holds a steady FPS at/above the D-Q6 target
  (measured over >100 frames) with no tearing/flicker.

## I-3 (MEDIUM): No automated visual/behavior verification

- *Symptom:* nothing asserts that a frame looks like what we think it does,
  or that button mappings produce the right game behavior. Emulator smoke
  ("it loads") is the only gate so far.
- *Desired:* a deterministic framebuffer-diff test (golden frame bytes), a
  button-mapping unit test, and a clock-accuracy test for `millis()` /
  `micros()`.
- *Acceptance:* `make test` (or the documented test command) passes on a
  clean checkout.

## I-4 (LOW): Border/chrome not yet implemented

The current window outline is always full-white and adds nothing in default
black-and-white mode. The *what* is settled: reserve a **title strip**, show a
**boxed Pause indicator**, and possibly a pause-time settings menu
([D-Q4](./DECISIONS.md)). The *who/cost* is now also settled: the framework
draws the chrome, re-inking only on change, with small game-tweak methods
([D-Q13](./DECISIONS.md)). Remaining work is ROADMAP step 8, not a design
question.

## I-5 (LOW): Tone-period edge case

`lynx_tone_square` clamps `count = 0` (impossible 1 MHz) low by ~4x rather
than failing loudly; the high end clamps at the lowest representable
frequency. Host-verified that all in-range periods are exact; the clamps are a
silent-accuracy gap worth a comment or warning.

## I-6 (LOW): EEPROM is volatile

The EEPROM shim is RAM-backed (1024 bytes), so game saves vanish on
power-off. Inherent to the Lynx, but undocumented for users who expect
persistence. Document it and consider a cartridge-RAM-backed option.

## I-7 (INFO): Zero-page / BLL layout trap

- **Fixed** in the current link script, but easy to regress: initialized
  `.zp.data` becoming `payload[0]` boots into the BIOS "INSERT GAME" loop.
- Add a build-time assertion that `payload[0]` is `_start` and a smoke test
  that enforces it. See the "Zero-page data trap" in [NOTES.md](./NOTES.md).

## I-8 (MEDIUM): FX-chip-backed assets have no Lynx equivalent

- *Symptom:* Arduboy games routinely page graphics/sound from the optional
  FX chip's 4 MB flash (`FX` read APIs, `Sprites` `SpritesB` data). The Lynx
   has no FX chip and BLL images are RAM-resident, so such assets cannot be
  played back as-is.
- *Something keeps the RAM image bounded anyway:* any stock Arduboy sketch +
  data already fits 32 KB flash (D-Q11).
- *Direction:* build-time guard that flags FX-API usage / oversized constant
  data and fails the compile with a clear message
  ([D-Q11](./DECISIONS.md)); long-term substitute for FX flash is still open
  ([Q-16](OPEN_QUESTIONS.md#q-16)).