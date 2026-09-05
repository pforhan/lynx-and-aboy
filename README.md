# lynx-and-aboy

**Getting Arduboy software and games running on the Atari Lynx.**

This project provides an alternate hardware backend for the
[Arduboy2](https://github.com/MLXXXp/Arduboy2) library (BSD-3-Clause, same as
this repo). Sketches written against the standard Arduboy2 API compile and run
**unchanged** on a real Atari Lynx or the Handy emulator, using the
[llvm-mos](https://github.com/llvm-mos/llvm-mos-sdk) SDK as the C++ toolchain.

It works by keeping the entire platform-neutral game engine from Arduboy2
(drawing, sprites, text, frame pacing) and supplying a newly written
hardware layer — `Arduboy2Core` plus a small Arduino-compatible shim — that
drives the Lynx's Mikey/Suzy chips instead of the Atmega32u4/SSD1306.

## Features

- 128x64 Arduboy game buffer centered on the Lynx's 160x102 panel
  (default: black & white, like an Arduboy; 59.9 FPS).
- **Lynx-only bonus (auto-gated by `#ifdef __LYNX__`):**
  - **Option 1** switches to a rich 16-color palette instead of B/W.
  - **Pause** button pops up a `PAUSED` overlay and halts the game until
    pressed again.
- Double-buffered via `DISPADR` swapping at the frame boundary (no tearing).
- Buttons mapped from the Lynx keypad (Up/Down/Left/Right + A/B); the
  LEFTHAND/SRSYS swap is honored for left-handed units.
- A `main()` entry is provided so a plain `.ino` compiles directly for the
  Lynx; the very same `.ino` builds as a normal Arduboy sketch.

## Requirements

### Building for Lynx

- **llvm-mos SDK** — the `mos-lynx-bll` target. Install a prebuilt release or
  build from source. Anything >= clang 13 should work; this project is tested
  with clang 23 (`c798c31416f72b395c658b5502d281a162387ab1`). The toolchain
  binaries must be on your `PATH` or referenced by the build script.
  - Release + install instructions: <https://github.com/llvm-mos/llvm-mos-sdk>
- *(Optional)* **handy** — the Atari Lynx emulator, to run the generated
  `.bll.o` binaries on a PC (for example via `handy --rom symbol.l.x` or a BLL
  loader such as `bll`).

So no AVR cross-toolchain is needed for the Lynx target.
- *(Optional)* **Mednafen** (>= 1.29, ships a Lynx module) or **Handy** to run
  the image on a PC: the build also emits `build/lynx-demo.lnx` (a copy of
  the `.bll.o`) so `mednafen build/lynx-demo.lnx` auto-detects the Lynx module.
  F9 inside Mednafen writes an exact framebuffer PNG.

### Building for real Arduboy

- **Arduino IDE** (>= 1.8) or **arduino-cli**.
- The standard **Arduboy board package** and the **Arduboy2 library**
  (install via the Library Manager). The shared sample sketch is meant to be
  built with those; the Lynx port in this repo is not needed by the Arduboy
  build (the `__LYNX__` macro is not defined there, so the Lynx-only bonus
  code is compiled out).

## Building

### Lynx

```sh
cd arduboy-lynx
./build/build-lynx.sh examples/lynx-demo/lynx-demo.ino
```

This produces `build/lynx-demo.bll.o`. The BLL image there loads directly in
Handy/Mednafen (`-force_module lynx`) or pushes to real hardware over
ComLynx with the `bll` loader. The build uses a small custom link script
(`build/lynx-bll/link.ld`) that keeps the payload starting with `_start` so
the boot entry executes code (see NOTES.md).
The demo controls (all default Arduboy2 API) plus the Lynx-only bonus keys:

| Input        | Effect                                             |
| ------------ | -------------------------------------------------- |
| A / wall hits| short beeps (BeepPin1 -> Lynx audio channel A)     |
| UP / DOWN    | invert display on / off                            |
| **Option 1** | toggle rich Lynx color palette (Lynx only)         |
| **Pause**    | freeze with an overlay until released (Lynx only)  |

### Arduboy

Open `examples/<demo>/<demo>.ino` in the Arduino IDE (or use `arduino-cli
compile --fqbn arduino:avr:leonardo`), select the Arduboy board, verify, and
upload as usual. The Option 1 / Pause extras compile out on the Arduboy.

## Project layout

```
arduboy-lynx/
  src/                  The Arduboy2 engine, BSD-licensed and attributed.
                        Platform-neutral sources kept verbatim;
                        Arduboy2Core + Arduboy2Beep are Lynx implementations.
  platform/lynx/        Arduino-compatible shim + Lynx hardware driver
                        (video, timers, keypad, audio, EEPROM-in-RAM, Print).
  examples/             A shared sketch that builds for both platforms.
  build/                Build scripts for Lynx and (via arduino-cli) Arduboy.
```

See `NOTES.md` for the register-level research this port is based on.

## How it works (the short version)

| Arduboy (AVR/SSD1306)      | Lynx (Mikey/Suzy)                        |
| -------------------------- | ---------------------------------------- |
| SPI write to OLED          | Comtinuous DMA from a 4 bpp framebuffer  |
| Upload 1 bpp buffer        | Expand sBuffer 1 bpp → 4 bpp, 2 buffers  |
| Timer0 millis()            | Mikey TIM4 (1 µs, polled)                 |
| Timer2 frame pacing        | Mikey TIM0/TIM2 → VBL 59.9 FPS           |
| Digital button reads       | Suzy `$FCB0`/`$FCB1` keypad (active high) |
| Speaker pins (2 tones)     | Mikey square-wave audio channels         |
| EEPROM                     | Emulated in RAM                          |

No interrupts are used anywhere; everything is busy-polling, exactly like the
classic Arduboy approach.

## License

BSD 3-Clause (see `LICENSE`). The Arduboy2 library portions are
Copyright (c) 2016-2021, Scott Allen, forked from the Arduboy library
(Copyright (c) 2016, Kevin Bates / Chris Martinez / Josh Goebel / Scott Allen)
and used under the same BSD 3-Clause terms; their copyright notices are
retained in `arduboy-lynx/src/`.

## Status

The Lynx port builds the shared demo end-to-end (`build/build-lynx.sh` →
`lynx-demo.bll.o`, ~8 KB, zero warnings), and the image already loads and
emulates in recent Mednafen. Remaining: an eyeball check of the demo on an
emulator and on real hardware (see the manual checklist in `NOTES.md`), plus
the follow-up ArduboyTones port.