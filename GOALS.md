# Goals

The "why" of this project. Discrete work items that serve these goals live in
[ROADMAP.md](./ROADMAP.md); each roadmap item cites the `G-`/`Q-`/`I-`
identifier it serves.

## G-1 (mission): Seamless Arduboy → Lynx porting

Provide a low-friction way to port Arduboy games to the Atari Lynx with
**minimal or zero source changes**. Sketches written against the standard
Arduboy2 API should compile and run on real Lynx hardware or emulators as-is.
"Seamless" is now concretely defined by
[D-Q1](./DECISIONS.md#d-q1-ship-an-arduboylx-api-layer-was-q-1): provide an
**ArduboyLx** API that falls back to standard Arduboy semantics on the
Arduboy platform, with the acceptance test *any sketch that builds unmodified
on a stock Arduboy also builds unmodified for the Lynx and is playable*.
That "as-is" target covers **both** classic `Arduboy` and `Arduboy2` API
families (Q-2 answer), and the Lynx-first surface is a native 4bpp
palette-indexed framebuffer exposed through methods, with double-buffering
hidden ([D-Q12](./DECISIONS.md#d-q12)).

## G-2 (API fidelity): Keep the engine verbatim

Keep the platform-neutral Arduboy2 engine verbatim; only the hardware layer
(`Arduboy2Core`, audio, the Arduino shim) is replaced. This keeps upstream
parity easy and bug-for-bug compatible. One sanctioned carve-out
([D-Q9](./DECISIONS.md)): the sprite *renderer* may use Suzy's native
SPRDISP/SPRCTL path so long as the `Sprites` *API* surface stays verbatim and
a portable software fallback (`SpritesLynx.cpp`) remains.

## G-3 (no-interrupt philosophy): Match the Arduboy model

No interrupts anywhere — everything busy-polls, exactly like the classic
Arduboy approach. Simple and portable.

## G-4 (Lynx-native extras without breaking Arduboy)

Provide Lynx-only bonuses (rich 16-color palette, chrome around the game
window, boxed pause indicator) via `#ifdef __LYNX__` so the *same* `.ino`
still builds as a stock Arduboy sketch with the bonus code compiled out. The
chrome direction is settled by
[D-Q4](./DECISIONS.md#d-q4-chrome--title-strip--boxed-pause--settings-was-q-4):
title space plus boxed pause overlay, with a possible pause-time settings
menu; ownership is hybrid — framework-drawn with small game-tweak methods
([D-Q13](./DECISIONS.md#d-q13)).

## G-5 (evidence-based port): No guessed register values

Every register value, timer chain, and mapping is derived from primary
sources (MonLynx hardware docs) and verified in an emulator and on real
hardware, not assumed. See [NOTES.md](./NOTES.md) for what's verified so far.

## G-6 (usable tooling): One obvious command

The build must be a single, obvious command with clear help, good error
messages, and a toolchain pre-flight check, so a newcomer can go from
clone → running game without digging.

## Status

- G-1..G-5 are **partly met**: the core port builds end-to-end and loads in an
  emulator, but the color issue ([I-1](KNOWN_ISSUES.md#i-1)) likely stems from
  the unprogrammed master palette (Q-7) and the frame rate
  ([I-2](KNOWN_ISSUES.md#i-2)) still blocks the "playable as-is" claim. Design direction is settled for the
  ArduboyLx API ([D-Q1](./DECISIONS.md), [D-Q12](./DECISIONS.md)), chrome
  ([D-Q4](./DECISIONS.md), [D-Q13](./DECISIONS.md)), sprites
  ([D-Q9](./DECISIONS.md)), display substrate ([D-Q10](./DECISIONS.md)) and the
  size/FX guard ([D-Q11](./DECISIONS.md)); the ArduboyLx surface is scaffolded
  in ROADMAP step 9.
- G-6 is **not yet met**; the build scripts exist but have no help/error
  handling or pre-flight check, and no image-budget guard (ROADMAP steps 2-3).