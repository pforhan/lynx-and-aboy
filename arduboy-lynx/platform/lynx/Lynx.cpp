/*
  lynx-and-aboy: Lynx hardware layer implementation.

  Only a small slice of Mikey/Suzy is used:
    - TIM0+TIM2 : video frame clock (~59.9 FPS, 159us * 105 lines).
    - TIM4      : 1us free-running counter backing millis()/micros().
    - Audio ch  : polynomial-counter square wave.
    - $FCB0/$FCB1/$FC92 : keypad + pause + orientation.
*/

#include "Lynx.h"

#include <string.h>

// ---------------------------------------------------------------------------
// Video
// ---------------------------------------------------------------------------

void lynx_video_init(uint8_t *buf)
{
  // Zero the frame buffer so the panel comes up cleanly.
  memset(buf, 0, 160 * 102 / 2);

  // Horizontal: 1us clock, reload+count, backup 158 -> 159us per line.
  LYNX_TBKUP(0) = 158;
  LYNX_TCTLA(0) = LYNX_CTL_RELOAD | LYNX_CTL_COUNT | LYNX_CTL_CLK_1US;

  // Vertical: linked from TIM0, reload+count, backup 104 -> 105 lines.
  // INTEN is required: the emulator and real Mikey only raise the INTRST
  // flag for a timer if its interrupt-enable bit is set (see mikie.cpp
  // DisplayEndOfFrame), and lynx_wait_for_frame() polls that flag.
  LYNX_TBKUP(2) = 104;
  LYNX_TCTLA(2) =
    LYNX_CTL_INTEN | LYNX_CTL_RELOAD | LYNX_CTL_COUNT | LYNX_CTL_CLK_LINK;

  // P-count for 60Hz.
  LYNX_PBKUP = 41;

  // Color, 4bpp, no flip, DMA on.
  LYNX_DISPADR_L = (uint16_t)buf & 0x00FF;
  LYNX_DISPADR_H = ((uint16_t)buf >> 8) & 0x00FF;
  LYNX_DISPCTL = LYNX_DISPCTL_COLOR | LYNX_DISPCTL_FOURBIT | LYNX_DISPCTL_ENABLE;
}

void lynx_swap_display(const uint8_t *buf)
{
  lynx_wait_for_frame();
  LYNX_DISPADR_L = (uint16_t)buf & 0x00FF;
  LYNX_DISPADR_H = ((uint16_t)buf >> 8) & 0x00FF;
}

// ---------------------------------------------------------------------------
// Clock
// ---------------------------------------------------------------------------

// The Lynx has no free-running millisecond clock we can trust reliably, so we
// count screen refreshes: each VBL from the vertical line counter (TIM2) is
// one frame, and the panel runs at 75 VBLs/second.
//
// IMPORTANT: the VBL flag is also consumed by lynx_wait_for_frame()/swap, so
// the shared counter below is the single source of truth. Polling it here
// (from millis()) decouples the clock from the render/swap path -- otherwise
// the clock can never advance until a frame is rendered, and a frame can
// never be rendered until the clock advances.

static unsigned long vblCount = 0;

// INTRST bit 2 = TIM2 (vertical line counter) timeout.
#define LYNX_VBL_FLAG 0x04

// Poll the VBL done flag once; folds any new frame into vblCount.
static void poll_vbl(void)
{
  if (LYNX_INTRST & LYNX_VBL_FLAG)
  {
    LYNX_INTRST = LYNX_VBL_FLAG;
    vblCount++;
  }
}

void lynx_wait_for_frame(void)
{
  // Wait until at least one *new* VBL has been counted, so the renderer
  // paces itself against the display refresh even if it races the clock.
  unsigned long target = vblCount + 1;
  unsigned long t0 = lynx_millis();
  uint16_t spins = 0;

  while (vblCount < target)
  {
    poll_vbl();

    // Never stall the renderer: bail if a frame edge hasn't latched within
    // 200ms of clock time, or after a CPU spin cap.
    if ((unsigned long)(lynx_millis() - t0) >= 200UL)
    {
      return;
    }
  } while (++spins != 0);
}

#define CLK_PERIOD_US 256UL
#define CLK_TIMER 4

static unsigned long microsBase;      // us accounted at poll boundaries


void lynx_clock_init(void)
{
  microsBase = 0;
  vblCount = 0;

  LYNX_TBKUP(CLK_TIMER) = 255;
  LYNX_TCTLA(CLK_TIMER) =
    (LYNX_CTL_RELOAD | LYNX_CTL_COUNT | LYNX_CTL_CLK_1US);

  // Absorb the first (possibly stale) done flag now.
  (void)LYNX_TCTLB(CLK_TIMER);
  LYNX_TCTLA(CLK_TIMER) |= LYNX_CTL_RESET_DONE;
  LYNX_TCTLA(CLK_TIMER) &= (uint8_t)~LYNX_CTL_RESET_DONE;
}

// Poll TIM4's done bit and fold any completed period(s) into microsBase.
static void clock_catch_up(void)
{
  while (LYNX_TCTLB(CLK_TIMER) & LYNX_TMR_DONE)
  {
    LYNX_TCTLA(CLK_TIMER) |= LYNX_CTL_RESET_DONE;
    LYNX_TCTLA(CLK_TIMER) &= (uint8_t)~LYNX_CTL_RESET_DONE;
    microsBase += CLK_PERIOD_US;
  }
}

unsigned long lynx_micros(void)
{
  clock_catch_up();

  // The down counter runs backup..0; elapsed in the current period grows as
  // the counter falls, keeping the result monotonic across done flips.
  return microsBase + (255 - LYNX_TCNT(CLK_TIMER));
}

unsigned long lynx_millis(void)
{
  poll_vbl();

  // 75 VBLs per second -> 1 VBL = 13.333 ms
  return (vblCount * 1000UL) / 75UL;
}

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------

static uint8_t audVol[4] = {0x40, 0x40, 0x40, 0x40};
static uint8_t audMuted = 0;

// Apply the programmed volume, honoring the master mute.
static void audApplyVol(uint8_t ch)
{
  LYNX_AUDV(ch) = audMuted ? 0 : audVol[ch];
}

void lynx_audio_mute(uint8_t muted)
{
  audMuted = muted;
  for (uint8_t ch = 0; ch < 4; ch++)
  {
    audApplyVol(ch);
  }
}

void lynx_tone_square(uint8_t ch, uint32_t pUs)
{
  uint8_t clockSel = LYNX_CTL_CLK_1US;   // 1us per audio clock
  uint32_t p;

  // The output toggles once per audio clock, so the audio timer's period is
  // P * (B+1) = pUs/2. Find the smallest prescaler that keeps B within range.
  // Prefer a prescaler whose maximum (P*256) can cover pUs/2.
  p = pUs >> 1;
  if (p < 2)
  {
    p = 1;
    clockSel = LYNX_CTL_CLK_1US;
  }
  else if (p <= 256)
  {
    clockSel = LYNX_CTL_CLK_1US;
  }
  else if (p <= 512)
  {
    clockSel = LYNX_CTL_CLK_2US;
    p >>= 1;
  }
  else if (p <= 1024)
  {
    clockSel = LYNX_CTL_CLK_4US;
    p >>= 2;
  }
  else if (p <= 2048)
  {
    clockSel = LYNX_CTL_CLK_8US;
    p >>= 3;
  }
  else if (p <= 4096)
  {
    clockSel = LYNX_CTL_CLK_16US;
    p >>= 4;
  }
  else if (p <= 8192)
  {
    clockSel = LYNX_CTL_CLK_32US;
    p >>= 5;
  }
  else
  {
    clockSel = LYNX_CTL_CLK_64US;
    p >>= 6;
  }

  if (p > 256)
  {
    p = 256;                       // clamp to the lowest representable frequency
  }
  if (p < 2)
  {
    p = 2;                         // shifter needs at least a 2-clock period
  }

  // Stop the channel's counter before touching the shifter.
  LYNX_AUDCTL(ch) = 0;

  // Poly counter: perfect square wave (feedback tap on bit 0, seed 1).
  LYNX_SHIFTLO(ch) = 0x01;
  LYNX_SHIFTHI(ch) = 0x00;
  LYNX_AUDFB(ch) = 0x01;
  LYNX_AUDOUT(ch) = 0x00;

  // Program the period, then (re)start the clock.
  LYNX_AUDTIM(ch) = (uint8_t)(p - 1);
  LYNX_AUDCTL(ch) = LYNX_CTL_RELOAD | LYNX_CTL_COUNT | clockSel;
  audApplyVol(ch);
}

void lynx_tone_volume(uint8_t ch, uint8_t vol)
{
  audVol[ch & 3] = vol;
  audApplyVol(ch & 3);
}

void lynx_tone_stop(uint8_t ch)
{
  LYNX_AUDCTL(ch) = 0;
  LYNX_AUDV(ch) = 0x00;
}

// ---------------------------------------------------------------------------
// Buttons
// ---------------------------------------------------------------------------

uint8_t lynx_keypad(void)
{
  return LYNX_JOYSTICK;
}

uint8_t lynx_pause_key(void)
{
  return LYNX_SWITCHES & 0x01;
}

uint8_t lynx_left_handed(void)
{
  return (LYNX_SPRSYS & 0x08) != 0;
}