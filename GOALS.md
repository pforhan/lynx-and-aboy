# Goals

The "why" of this project. Discrete work items that serve these goals live in
[ROADMAP.md](./ROADMAP.md); each roadmap item cites the `G-`/`Q-`/`I-`
identifier it serves.

## G-1 (mission): Seamless Arduboy → Lynx porting

Provide a low-friction way to port Arduboy games to the Atari Lynx with
**minimal or zero source changes**. Sketches written against the standard
Arduboy2 API should compile and run on real Lynx hardware or emulators as-is.
What "seamless" means precisely is still being decided — see
[Q-1](OPEN_QUESTIONS.md#q-1).

## G-2 (API fidelity): Keep the engine verbatim

Keep the platform-neutral Arduboy2 engine verbatim; only the hardware layer
(`Arduboy2Core`, audio, the Arduino shim) is replaced. This keeps upstream
parity easy and bug-for-bug compatible.

## G-3 (no-interrupt philosophy): Match the Arduboy model

No interrupts anywhere — everything busy-polls, exactly like the classic
Arduboy approach. Simple and portable.

## G-4 (Lynx-native extras without breaking Arduboy)

Provide Lynx-only bonuses (rich 16-color palette, pause overlay, border
chrome) via `#ifdef __LYNX__` so the *same* `.ino` still builds as a stock
Arduboy sketch with the bonus code compiled out.

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
  emulator, but the two HIGH issues (color, frame rate) block the "playable
  as-is" claim.
- G-6 is **not yet met**; the build scripts exist but have no help/error
  handling or pre-flight check.