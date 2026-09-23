# Contributing

Deliberately scoped, good-first-issue candidates. Each maps to a goal, open
question, or known issue; details and acceptance criteria are in
[ROADMAP.md](./ROADMAP.md).

- Survey real Arduboy games for direct `sBuffer` access — evidence for
  [Q-2](./OPEN_QUESTIONS.md#q-2). (ROADMAP step 6.)
- Program the master palette and render a 16-slot test grid — fires at
  [I-1](./KNOWN_ISSUES.md#i-1) / [Q-7](./OPEN_QUESTIONS.md#q-7). (ROADMAP
  step 5.)
- Golden-frame + button unit tests on the host — fixes
  [I-3](./KNOWN_ISSUES.md#i-3). (ROADMAP step 4.)
- Toolchain pre-flight check script — serves
  [G-6](./GOALS.md#g-6-usable-tooling-one-obvious-command). (ROADMAP step 3.)
- Build-time `payload[0] == _start` + image-size/FX guard — hardens
  [I-7](./KNOWN_ISSUES.md#i-7) and [D-Q11](./DECISIONS.md). (ROADMAP step 2.)
- Spike driving `Sprites` through Suzy's SPRDISP path — implements
  [D-Q9](./DECISIONS.md) / answers [Q-14](./OPEN_QUESTIONS.md#q-14).
  (ROADMAP step 7.)
- Scaffold the `ArduboyLx.h` API layer — implements
  [D-Q1](./DECISIONS.md) and [D-Q12](./DECISIONS.md). (ROADMAP step 9.)
- Hybrid chrome (framework-drawn, game-tweakable) — implements
  [D-Q13](./DECISIONS.md) / closes [I-4](./KNOWN_ISSUES.md#i-4). (ROADMAP
  step 8.)
- `ArduboyTones` audio path in ArduboyLx — implements
  [D-Q5](./DECISIONS.md#d-q5-support-both-beep-and-arduboytones-in-arduboylx-was-q-5).
  (ROADMAP step 10.)

## How to run things

- Build the Lynx demo: `cd arduboy-lynx && ./build/build-lynx.sh
  examples/lynx-demo/lynx-demo.ino` → `build/lynx-demo.bll.o` and
  `build/lynx-demo.lnx`.
- Run host tests (once they exist): see ROADMAP step 4 for the command.
- Manual emulator + hardware checklist: see `NOTES.md`.

## Process

- Reference the relevant `G-`/`Q-`/`I-`/`D-` identifier in the PR description.
- When a question in [OPEN_QUESTIONS.md](./OPEN_QUESTIONS.md) is settled,
  move it to [DECISIONS.md](./DECISIONS.md) (as `D-Q*`) rather than deleting
  it or marking it in place, so the decision history survives in one place.