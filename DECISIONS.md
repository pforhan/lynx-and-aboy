# Decisions

Settled design decisions, moved here from
[OPEN_QUESTIONS.md](./OPEN_QUESTIONS.md). Each keeps its original `Q-` id
(e.g. **D-Q1** was formerly `Q-1`) so the rest of the docs can reference the
decision history without ambiguity. The rationale and any dissent are recorded
here; the *effect* on code, issues, and roadmap is disseminated in the file
each decision points to.

Date recorded: 2026-09-08.

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
- **Effect:** [ROADMAP step 9](./ROADMAP.md) becomes implementing the
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
- **Effect:** [ROADMAP step 11](./ROADMAP.md) is downgraded from a gate to an
  optional pass.