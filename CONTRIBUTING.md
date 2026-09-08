# Contributing

Deliberately scoped, good-first-issue candidates. Each maps to a goal, open
question, or known issue; details and acceptance criteria are in
[ROADMAP.md](./ROADMAP.md).

- Survey real Arduboy games for direct `sBuffer` access — evidence for
  [Q-2](./OPEN_QUESTIONS.md#q-2). (ROADMAP step 6.)
- Golden-frame + button unit tests on the host — fixes
  [I-3](./KNOWN_ISSUES.md#i-3). (ROADMAP step 4.)
- Toolchain pre-flight check script — serves
  [G-6](./GOALS.md#g-6-usable-tooling-one-obvious-command). (ROADMAP step 3.)
- Build-time `payload[0] == _start` assertion — hardens
  [I-7](./KNOWN_ISSUES.md#i-7). (ROADMAP step 2.)
- `ArduboyTones` port (deferred follow-up) — informs
  [Q-5](./OPEN_QUESTIONS.md#q-5). (ROADMAP step 9.)

## How to run things

- Build the Lynx demo: `cd arduboy-lynx && ./build/build-lynx.sh
  examples/lynx-demo/lynx-demo.ino` → `build/lynx-demo.bll.o` and
  `build/lynx-demo.lnx`.
- Run host tests (once they exist): see ROADMAP step 4 for the command.
- Manual emulator + hardware checklist: see `NOTES.md`.

## Process

- Reference the relevant `G-`/`Q-`/`I-` identifier in the PR description.
- When closing a `Q-`/`I-` item, mark it resolved in place (don't delete) in
  the corresponding file so the decision history survives.