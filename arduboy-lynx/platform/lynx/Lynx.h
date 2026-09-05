/*
  lynx-and-aboy: minimal, interrupt-free driver for the Atari Lynx
  (Mikey + Suzy) as seen from the llvm-mos SDK.

  All register values/offsets verified against MonLynx "hardware.html":
    - Timers: TIM0..TIM7 at $FD00-$FD1F (BKUP, CTLA, CNT, CTLB each).
    - Video: DISPCTL $FD92, PBKUP $FD93, DISPADR $FD94/$FD95.
    - Joystick $FCB0 (active HIGH for pressed), SWITCHES $FCB1 (pause bit 0).
    - Joystick LEFTHAND orientation bit: SPRSYS $FC92 bit 3.
    - Audio channels A-D at $FD20/$FD28/$FD30/$FD38.
*/

#ifndef LYNYN_LYNX_H
#define LYNYN_LYNX_H

#include <stdint.h>

// ---------------------------------------------------------------------------
// Register map
// ---------------------------------------------------------------------------

#define LYNX_JOYSTICK (*(volatile uint8_t *)0xFCB0)
#define LYNX_SWITCHES (*(volatile uint8_t *)0xFCB1)
#define LYNX_SPRSYS (*(volatile uint8_t *)0xFC92)

#define LYNX_INTRST (*(volatile uint8_t *)0xFD80)

#define LYNX_DISPCTL (*(volatile uint8_t *)0xFD92)
#define LYNX_PBKUP (*(volatile uint8_t *)0xFD93)
#define LYNX_DISPADR_L (*(volatile uint8_t *)0xFD94)
#define LYNX_DISPADR_H (*(volatile uint8_t *)0xFD95)

// Timer channel base addresses (8 channels x 4 bytes)
#define LYNX_TIM(n) (0xFD00 + ((n) & 7) * 4)
#define LYNX_TBKUP(n) (*(volatile uint8_t *)(LYNX_TIM(n) + 0))
#define LYNX_TCTLA(n) (*(volatile uint8_t *)(LYNX_TIM(n) + 1))
#define LYNX_TCNT(n) (*(volatile uint8_t *)(LYNX_TIM(n) + 2))
#define LYNX_TCTLB(n) (*(volatile uint8_t *)(LYNX_TIM(n) + 3))

// Timer CTLA bits
#define LYNX_CTL_INTEN 0x80
#define LYNX_CTL_RESET_DONE 0x40
#define LYNX_CTL_MAGMODE 0x20
#define LYNX_CTL_RELOAD 0x10
#define LYNX_CTL_COUNT 0x08
#define LYNX_CTL_CLK_LINK 0x07
#define LYNX_CTL_CLK_64US 0x06
#define LYNX_CTL_CLK_32US 0x05
#define LYNX_CTL_CLK_16US 0x04
#define LYNX_CTL_CLK_8US 0x03
#define LYNX_CTL_CLK_4US 0x02
#define LYNX_CTL_CLK_2US 0x01
#define LYNX_CTL_CLK_1US 0x00

// Timer CTLB bit 3 = "done" (timer borrow / timeout)
#define LYNX_TMR_DONE 0x08

// Audio channel base addresses
#define LYNX_AUD(n) (0xFD20 + ((n) & 3) * 8)
#define LYNX_AUDV(n) (*(volatile uint8_t *)(LYNX_AUD(n) + 0))
#define LYNX_AUDFB(n) (*(volatile uint8_t *)(LYNX_AUD(n) + 1))
#define LYNX_AUDOUT(n) (*(volatile uint8_t *)(LYNX_AUD(n) + 2))
#define LYNX_SHIFTLO(n) (*(volatile uint8_t *)(LYNX_AUD(n) + 3))
#define LYNX_AUDTIM(n) (*(volatile uint8_t *)(LYNX_AUD(n) + 4))
#define LYNX_AUDCTL(n) (*(volatile uint8_t *)(LYNX_AUD(n) + 5))
#define LYNX_AUDCNT(n) (*(volatile uint8_t *)(LYNX_AUD(n) + 6))
#define LYNX_SHIFTHI(n) (*(volatile uint8_t *)(LYNX_AUD(n) + 7))

// Master stereo / output enables (1 = channel output disabled)
#define LYNX_MSTERE0 (*(volatile uint8_t *)0xFD50)

// Display control bits
#define LYNX_DISPCTL_COLOR 0x08
#define LYNX_DISPCTL_FOURBIT 0x04
#define LYNX_DISPCTL_FLIP 0x02
#define LYNX_DISPCTL_ENABLE 0x01

// ---------------------------------------------------------------------------
// Timer / clock services
// ---------------------------------------------------------------------------

// Video: configure the TIM0 (horizontal) + TIM2 (vertical) chain and enable
// the display DMA against `buf` (must be 4-byte aligned).
void lynx_video_init(uint8_t *buf);

// Swap the DISPADR pointer to `buf` at the next vertical blanking boundary.
void lynx_swap_display(const uint8_t *buf);

// Block until the vertical-line counter (TIM2) encounters a new frame boundary.
void lynx_wait_for_frame(void);

// millis()/micros() clocking, driven by TIM4 (unused otherwise on the Lynx).
void lynx_clock_init(void);
unsigned long lynx_micros(void);
unsigned long lynx_millis(void);

// ---------------------------------------------------------------------------
// Audio (square wave via the polynomial counter, see NOTES.md)
// ---------------------------------------------------------------------------

// Play a square wave on channel `ch` (0..3) with a square-wave period of
// `pUs` microseconds. The LFSR bit0 toggles every audio clock, so the audio
// timer (backup+1) * prescaler picks P*(B+1) = pUs/2.
void lynx_tone_square(uint8_t ch, uint32_t pUs);

// Mute/unmute channel (volume to 0 or to `vol`).
void lynx_tone_volume(uint8_t ch, uint8_t vol);
void lynx_tone_stop(uint8_t ch);

// Master mute: silences all channels by setting volume to 0 while tones keep
// running; unmute restores each channel's programmed volume.
void lynx_audio_mute(uint8_t muted);

// ---------------------------------------------------------------------------
// Double-buffered framebuffer management (provided by main.cpp)
// ---------------------------------------------------------------------------

// Pointer to the buffer being rendered into (the non-visible one).
uint8_t *lynx_back_fb(void);

// Pointer to the buffer DISPADR currently points at (the visible one).
uint8_t *lynx_front_fb(void);

// After a swap, mark the newly-visible buffer as front so the next render
// targets the other one.
void lynx_front_index(void);

// ---------------------------------------------------------------------------
// Buttons
// ---------------------------------------------------------------------------

// Raw keypad read ($FCB0); active-high for pressed keys.
uint8_t lynx_keypad(void);

// Pause switch ($FCB1 bit 0), active high.
uint8_t lynx_pause_key(void);

// Orientation: 1 = "left hand" (the normal Atari orientation). The joystick
// bits physically swap when not left-handed.
uint8_t lynx_left_handed(void);

#endif // LYNYN_LYNX_H