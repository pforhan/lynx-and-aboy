# Development notes

Working notes for porting the Arduboy2 library to the Atari Lynx using the
llvm-mos SDK. Not user-facing documentation (see README.md).

## Strategy

- **Keep the platform-neutral Arduboy2 game engine close to verbatim.**
  `Arduboy2.h`, `Arduboy2.cpp`, `Arduboy2Data.cpp`, `Sprites*.h`, etc. are
  mostly platform-neutral and call only the Arduino API (`millis()`, etc.)
  plus static `Arduboy2Core`/shim functions. Two AVR asm hot-spots exist in
  `Arduboy2.cpp` (`drawPixel`, `fillScreen`) and are wrapped with
  `#if defined(__AVR__)` portable fallbacks; the AVR path is byte-identical.
  The AVR `Sprites.cpp` class counterpart is replaced by `SpritesLynx.cpp`.
- **Replace the hardware layer.** `Arduboy2Core.h/.cpp`,
  `Arduboy2Audio.cpp`, `Arduboy2Beep.cpp` get a Lynx implementation.
- **Provide an Arduino-compatible shim** so the verbatim files compile for the
  Lynx: `Arduino.h`, `Print.h/cpp`, `EEPROM.h/cpp`, and AVR stub headers
  (`avr/pgmspace.h`, `avr/power.h`, `avr/sleep.h`, `avr/wdt.h`,
  `avr/interrupt.h`, `avr/io.h`).
- Licensing: Arduboy2 is **BSD 3-clause** (same as this repo), so carrying the
  upstream files with attribution is clean. Keep copyright notices.

## Toolchain (verified locally)

- Installed at `/home/pforhan/bin/lynxdev/llvm-mos/`, clang 23.0.0git commit
  `c798c31416f72b395c658b5502d281a162387ab1`.
- Driver: `mos-lynx-bll-clang` (C) / `mos-lynx-bll-clang++` (C++).
- Config chain: `bin/mos-lynx-bll.cfg` → `@mos-lynx.cfg` → `@mos-common.cfg`.
  - `mos-lynx.cfg`: `-isystem .../mos-platform/lynx/include`, `-mcpu=mos65c02`,
    `-D__LYNX__`, `-mlto-zp=224`. **`__LYNX__` is auto-defined** → use
    `#ifdef __LYNX__` for Lynx-only bonus code in the shared sample.
  - `mos-common.cfg`: `--target=mos`, `-flto`.
- Default `-flto` (whole-program LTO). Code-gen quirks are expected; keep
  volatile access to hardware registers.
- Link script `mos-platform/lynx-bll/lib/link.ld`:
  - zp: `0x20..0xFF`, ram: `0x200..0xFC00` (`__hiram_start = min(0xFC00, ...)`).
  - Output format: BLL loader header — `SHORT(0x0880)`, start address BE,
    length BE (includes the 10-byte header), `BS93`, then image. CPU starts
    at `0x0200` (Boot entry = payload[0] = file byte 10).
  - Load semantics (Mednafen Beetle Lynx `CRam`, HEADER_RAW_SIZE=10):
    `file[F] → RAM[start − 10 + F]`, boot PC = `start − 10`; byte 0 is the
    `80 08` (BRA rel+8) so execution always lands on payload[0] at RAM
    `start`. A valid image must therefore begin its payload with runnable
    code.
  - **Zero-page data trap:** the platform script places `.zp.data`
    (VMA `0x20`) with its load image in `ram` *first*, so any initialized
    zero-page data (small globals, `static const` arrays, string literals
    that the 65C02 backend zero-pages) lands at payload[0] — the emulator
    then executes data, triggers a BRK/RTI bounce, and the console boots the
    BIOS "INSERT GAME / power down" loop. Fixed by a **custom link script**
    (`arduboy-lynx/build/lynx-bll/link.ld`, used by `build-lynx.sh` via an
    explicit `ld.lld`): text → rodata → data → **zp image** → bss → noinit,
    keeping payload[0] = `_start`. The header length is made to cover the
    whole image with `__image_lma_end = MAX(__data_end, LOADADDR(.zp.data)
    + SIZEOF(.zp.data))`. (alpha-kinetics' stock build only worked because
    its `.zp.data` was empty.)
  - `build-lynx.sh` compiles each TU with the driver (`mos-lynx-bll-clang++`)
    then links with `ld.lld` + the custom script (driver's own `-Tlink.ld`
    resolves to the platform script via `-L` order and cannot be overridden;
    passing both `-T`s makes lld error "region 'zp' already defined").
- crt0 chain (lynx `libcrt0.a`): copy-zp-data + init-stack + zero-bss +
  exit-loop. `main()` called normally. `__copy_zp_data` still copies the
  reordered zp load image (`__zp_data_load_start` → `0x20`) at startup.
- Verified: a trivial `.c` compiles+links to a 97-byte `.bll.o` with correct
  header (`80 08 02 00 00 61 42 53 39 33 ...`).

## Lynx hardware reference (verified against MonLynx `hardware.html`)

Zero-page off = 0; all addresses consistent with MonLynx/cc65 usage.

### I/O registers (Mikey/Suzy at $FDxx-$FFxx)

- Joystick read: `$FCB0`, active-**HIGH** for pressed key. Bits:
  B7=Up, B6=Down, B5=Left, B4=Right, B3=Option1, B2=Option2, B1=Inner(A),
  B0=Outer(B). Pause key: `$FCB1` bit 0. Left-handed units swap the D-pad pairs.
- Display control `DISPCTL` `$FD92`: B3=color(1)/mono(0), B2=4bit(1)/2bit(0),
  B1=flip(1)=mirror-h, B0=video-DMA enable. Used values: **color 4bpp = `0x0D`**,
  mono 2bit = `0x05`.
- Display DMA start address `DISPADR` `$FD94`(lo)/`$FD95`(hi) → pointer into a
  framebuffer; hardware masks low 2 bits (4-byte alignment required).
  - 4bpp: 80 bytes/scanline (160px, even x = high nibble, odd x = low nibble).
    Framebuffer = 160×102×2 = **8160 bytes**. Scanline rows: 0..101.
  - Pixel (x,y) → byte `y*80 + (x>>1)`, nibble high if `x&1==0`.
    4-bit color is GRB + intensity (bit0=blue, bit1=red, bit2=green,
    bit3=brightness); **full white = `0x07`**, black = `0x00`.
  - Display is continuous DMA: render into a *back* buffer and swap `DISPADR`
    at frame boundary to avoid tearing.
- Mikey timers (`$FD00`-): each of TIM0..3 has BKUP,CTLA,CNT,CTLB.
  - TIMxCTLA: B7=int-enable `$80`, B6=reset-done-when-written `$40`, B4=reload
    `$10`, B3=count `$08`, clock select B2-0: 7=linking,6=64µs,5=32µs,4=16µs,
    3=8µs,2=4µs,1=2µs,0=1µs. **Writing B6 clears the CTLB "done" bit.**
  - TIMxCTLB: B3 set when a reload with no complement fires (borrow/done).
- Frame timing (drhelius lyny): TIM0: source 1µs, backup 158 → 159µs per
  scanline (`TIM0CTLA=0x18`); TIM2: source=TIM0, backup 104 → 105 lines/frame
  (`TIM2CTLA=0x1F`) → 16.695ms ≈ 59.9 FPS. `BSTR=41` (`$FD90`) irrelevant to us.
- Timer usage in port:
  - TIM0 + TIM2 → display frame rate / VBL sync (poll TIM2CTLB done bit;
    reset by writing B6 into TIM2CTLA).
  - TIM1 → millis()/micros() clock: source 1µs, backup 255, count+reload
    (`TIM1CTLA=0x18`). Poll TIM1CTLB bit3; on set, write `0x40` to TIM1CTLA,
    add 255µs to an accumulator (`microsAtLastBoundary`); return 0xFFFFFFFF if
    it ever rolls over (first wrap can't be within thousands of seconds).
- **Clock in the port uses TIM4** (the baud timer — serial is unused): source
  1µs, backup 255, reload+count (`TIM4CTLA = 0x18`). Poll TIM4CTLB bit3;
  on set, write `0x40` then clear it from TIM4CTLA, and add 256µs to
  `microsBase`. `micros() = microsBase + (255 - TIM4CNT)` keeps monotonic.
- Audio: channels A-D at `$FD20/$FD28/$FD30/$FD38` base offsets
  {AUDV, feedback, output, shiftreg, timer-BKUP, audio-ctrl, (other)}:
  `+0` volume, `+1` feedback, `+2` output, `+3` shiftreg lo, `+4` timer backup,
  `+5` audio control, `+7` other (B7=shiftreg bit11, B0=borrow).
- Square-wave recipe (derived): AUDV=volume (e.g. `0x40`), feedback=`0x02`,
  output=0, shiftreg seed=1, timer backup = half-period−1, audio control =
  reload|count|clock-select. Shift-register bit 0 toggles output → square wave
  at `timer_rate/2`. Choose prescaler 1µs..64µs so backup ∈ [1,255].

### Lynx display vs Arduboy coordinate mapping

- Real Lynx panel: 160×102. Games render in 4bpp to the full frame.
- The Arduboy game buffer is 128×64 (1bpp, 1024 bytes, vertical 8-bit columns).
- Port centers it: offset_x = 16, offset_y = 19 →
  game pixel (x,y) → Lynx pixel (16+x, 19+y).
- sBuffer byte layout: `sBuffer[(y>>3)*128 + x]`, bit0 = topmost pixel of the
  block. Matches Adafruit/SSD1306 layout (upstream `drawPixel`).
- Expansion 1bpp→4bpp is done row-wise (128px = 64 output bytes/line, 2 nibbles
  combined per byte) so neighboring columns (even/odd) share one framebuffer
  byte. Fast path = monochrome (on=`0x07`, off=`0x00`); rich-palette mode
  (Lynx-only, Option1) substitutes per-row ink colors.

### Feel / inputs

- Default button map (Arduboy units): Lynx → Arduboy
  Up→UP(`_BV(7)`), Down→DOWN(`_BV(4)`), Left→LEFT(`_BV(5)`),
  Right→RIGHT(`_BV(6)`), Inner(A)→A(`_BV(3)`), Outer(B)→B(`_BV(2)`).
  LEFTHAND is honored: Suzy **SPRSYS `$FC92` bit3** swaps Left↔Right when set.
- Lynx-only extras: OPTION1 and PAUSE exposed as constants
  (`#ifdef __LYNX__`). Option1 toggles rich-palette mode; Pause halts with an
  overlay until pressed again.

## Port layout

```
arduboy-lynx/
  src/                  Arduboy2 library (BSD, attributed)
    Arduboy2.h/cpp      verbatim except: drawPixel + fillScreen get an
                        #ifdef __AVR__ portable fallback (two asm blocks)
    Arduboy2Data.cpp    verbatim (font + logo, PROGMEM)
    Sprites.h, SpritesCommon.h, SpritesB.*   verbatim
    Sprites.cpp         AVR-only (giant SPI asm); NOT compiled for Lynx
    SpritesLynx.cpp     OURS: portable C reimplementation of the `Sprites`
                        class (same logic as upstream SpritesB.cpp)
    Arduboy2Audio.*     REPLACED with Lynx audio version (mute = vol 0)
    Arduboy2Core.h/cpp  REPLACED with Lynx implementation
    Arduboy2Beep.*      REPLACED with Lynx audio-channel version
  platform/lynx/        Arduino shim + Lynx hardware layer (all ours)
    Arduino.h/cpp, Print.h/cpp, EEPROM.h/cpp, Lynx.h/cpp, main.cpp
    avr/*.h             stub AVR headers (pgmspace, power, sleep, wdt, ...)
  examples/lynx-demo/   shared .ino (compiles for Arduboy or Lynx)
  build/                build-lynx.sh, build-arduboy.sh
```

## Build status (verified)

- `./arduboy-lynx/build/build-lynx.sh` compiles the demo end-to-end:
  `lynx-demo.bll.o` = **7978 bytes**, BLL header verified
  (`80 08 | 02 00 | 1f 2a | "BS93"`), **zero warnings** with `-Os -flto`.
  Payload[0] = `_start` (`a9 00 85 00 a9 fc 85 01 20 fb 07 ...`), so the
  boot entry runs code (previous builds put the zero-page load image first
  and the emulator executed data → BIOS "INSERT GAME" power-down loop).
- The image loads in **Mednafen 1.29.0** (`-force_module lynx`, Beetle Lynx
  core): detected as `lynx(Atari Lynx)`, runs indefinitely with no
  "System Halted / power down" resets. Rendered output still needs eyes
  (see manual checklist).
- The zero-page image (title/PAUSED strings, `richPal`, ball vars → copied
  to `0x20` at boot) now lives at file offset `0x1f04` (RAM ~`0x20fa`, after
  `.data`), not at the payload start.
- Tone-period math host-verified: for BeepPin1 `count` in 1..65534 and
  BeepPin2 `1..1023` every computed square frequency is **exact** (the
  prescaler division lands exactly) except the impossible 1 MHz edge
  (`count=0` clamps ~4x low) — see `platform/lynx/Lynx.cpp::lynx_tone_square`.
- The two asm blocks in `Arduboy2.cpp` (`drawPixel`, `fillScreen`) are wrapped
  in `#if defined(__AVR__)` with byte-identical AVR path; Lynx uses the C
  fallback. AVR build behavior is unchanged (#else is dead on AVR).
- All Lynx overrides (Arduboy2Core.cpp, Arduboy2Audio.cpp, Arduboy2Beep.cpp,
  SpritesLynx.cpp) are wrapped `#if !defined(__AVR__)` so installing the
  folder as a stock library can't double-define on AVR.

## Remaining TODO

- [x] Shannon shim (Arduino.h, Arduino.cpp, Print, EEPROM, avr stubs)
- [x] Core: boot/display/buttons/timers + paintScreen expansion
- [x] Beep → Lynx audio channels; Arduboy2Audio → mute via volume 0
- [x] Demo sample + build scripts (Lynx builds clean)
- [x] Emulator smoke test: builds + loads in Mednafen (`-force_module lynx`)
- [ ] Visual check of the demo in an emulator (manual, see checklist below)
- [ ] Verify `__LYNX__`-gated rich palette + pause bonus on hardware
- [ ] ArduboyTones port (separate MLXXXp library) — deferred follow-up
- [x] README finalize; NOTES → keep in sync

## Manual verification checklist (user-run)

Emulator (Mednafen 1.29.0 ships a Lynx module; Handy also fine):

```sh
./arduboy-lynx/build/build-lynx.sh arduboy-lynx/examples/lynx-demo/lynx-demo.ino
mednafen arduboy-lynx/build/lynx-demo.lnx
#   - the build emits lynx-demo.lnx (a copy of lynx-demo.bll.o) so Mednafen
#     auto-detects the Lynx module from the .lnx extension — no flag needed.
#   - if auto-detect ever fails, `-force_module lynx` still works; positional
#     "mednafen lynx file" does NOT.
#   - key map (`lynx.input.builtin.gamepad.*` in ~/.mednafen/mednafen.cfg,
#     SDL2 scancodes): A=A-key, B=B-key, D-pad=arrows, Option1="1",
#     Option2="2", Pause=Escape. Escape is Mednafen's conventional in-game
#     menu key too — re-bind `gamepad.pause` in the cfg if it gets swallowed.
#   - F9 (command.take_snapshot) dumps the EXACT 160x102 framebuffer PNG to
#     ~/.mednafen/snaps/ — best for pixel-perfect color checks (white=0x07).
```

What to look for:
- Boot: `Arduboy2.boot()` shows the **Arduboy logo** for ~2s, centered B/W.
- Then the demo: `LYNX AND A BOY` heading, a bouncing ball around (30,20).
- `UP`/`DOWN` toggle display invert (white↔black).
- `A` and each wall hit → short beeps (Lynx audio channels A; 880/1400/2000 Hz).
- Lynx-only (via the Lynx keypad map): **Option 1** cycles a rich 16-color
  palette; **Pause** freezes with a black `PAUSED` band *while held*.
- Any row-band artifacts, ac flicker, or wrong colors → renderer bug (double
  buffer / VBL swap in `Arduboy2Core.cpp`), report with a screenshot.

Hardware: push `arduboy-lynx/build/lynx-demo.bll.o` over ComLynx with the
`bll` loader; the panel should come up black (lynx_video_init zeroes fb) then
the same demo.
Verify Option1/Pause work on a real unit (the palette uses the Lynx 4-bit GRB
colors, which some emulators approximate differently).