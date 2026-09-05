/**
 * @file Arduboy2Core.cpp
 * \brief
 * Arduboy2Core implementation for the Atari Lynx.
 *
 * Renders the 128x64 1bpp Arduboy buffer into the Lynx's 160x102 4bpp
 * framebuffer (centered at x=16,y=19), double-buffered with a VBL-wait
 * DISPADR swap inside paintScreen(). See NOTES.md for the layout math.
 */

#include "Arduboy2Core.h"

#include "Lynx.h"

#if !defined(__AVR__)
// The whole of this file is the Lynx implementation of Arduboy2Core. On an
// AVR build the upstream Arduboy2Core.cpp (this file's full sibling) is used
// instead, so this translation unit must compile to nothing there.

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
// Lynx framebuffer: 160 x 102, 4bpp -> 80 bytes per display line.
#define FB_STRIDE 80
#define FB_H 102
#define WINDOW_X 16
#define WINDOW_Y 19
#define WINDOW_W 128
#define WINDOW_H 64

// Lynx 4bpp color is GRB (bit0=blue, bit1=red, bit2=green + intensity bit3).
#define INK_MONO 0x07      // full white
#define INK_OFF 0x00       // black

// Row-biased color so rich mode has some life beyond the Arduboy's 1bpp.
static const uint8_t richPal[16] = {
  0x07, 0x04, 0x06, 0x05,   // white, red, yellow, magenta
  0x01, 0x02, 0x03, 0x07,   // blue, green, cyan, white
  0x07, 0x03, 0x02, 0x01,   // white, cyan, green, blue
  0x05, 0x06, 0x04, 0x07    // magenta, yellow, red, white
};

uint8_t *lynx_back_fb(void);
void lynx_front_index(void);

// Renderer state
static bool invertFlag = false;
static bool hFlip = false;
static bool vFlip = false;
static bool richMode = false;
static uint8_t cursorX = 0;
static uint8_t cursorY = 0;

static uint8_t inkForRow(uint8_t r)
{
  if (richMode)
  {
    return richPal[r & 15];
  }
  return INK_MONO;
}

static inline uint8_t alphaPixel(uint8_t row, uint8_t on)
{
  // Returns the 4-bit pixel value for a logical on/off bit in a row.
  if (invertFlag)
  {
    on = !on;
  }
  return on ? inkForRow(row) : INK_OFF;
}

// ---------------------------------------------------------------------------
// Framebuffer fill helpers (operate on the *back* buffer, pre-swap)
// ---------------------------------------------------------------------------

static void fillWindow(uint8_t nibble, uint8_t y0, uint8_t y1)
{
  uint8_t *back = lynx_back_fb();
  uint8_t v = (uint8_t)((nibble << 4) | nibble);

  for (uint8_t r = y0; r < y1; r++)
  {
    uint8_t *p = back + (((uint16_t)WINDOW_Y + r) * FB_STRIDE) + WINDOW_X;
    for (uint8_t x = 0; x < WINDOW_W / 2; x++)
    {
      *p++ = v;
    }
  }
}

// ---------------------------------------------------------------------------
// Paint: row-major emit of the 128x64 1bpp buffer
// ---------------------------------------------------------------------------

void Arduboy2Core::paintScreen(uint8_t image[], bool clear)
{
  uint8_t *back = lynx_back_fb();
  uint8_t x, r;

  for (r = 0; r < WINDOW_H; r++)
  {
    uint8_t srcRow = vFlip ? (uint8_t)((WINDOW_H - 1) - r) : r;
    uint8_t bit = (uint8_t)(1U << (srcRow & 7));
    uint8_t *line = back + (((uint16_t)WINDOW_Y + r) * FB_STRIDE) + WINDOW_X;

    for (x = 0; x < WINDOW_W; x += 2)
    {
      uint8_t gx0 = x;
      uint8_t gx1 = (uint8_t)(x + 1);
      if (hFlip)
      {
        gx0 = (uint8_t)((WINDOW_W - 1) - x);
        gx1 = (uint8_t)((WINDOW_W - 1) - (x + 1));
      }

      uint8_t base = (uint8_t)((srcRow >> 3) * WINDOW_W);
      uint8_t p0 = image[base + gx0];
      uint8_t p1 = image[base + gx1];

      uint8_t v0 = alphaPixel(r, (p0 & bit) != 0);
      uint8_t v1 = alphaPixel(r, (p1 & bit) != 0);

      line[x >> 1] = (uint8_t)((v0 << 4) | v1);
    }
  }

  if (clear)
  {
    unsigned long n = (unsigned long)WINDOW_W * WINDOW_H / 8;
    uint8_t *p = image;
    while (n--)
    {
      *p++ = 0;
    }
  }

  // Double buffer: show this frame at the next VBL, then flip role.
  lynx_swap_display(back);
  lynx_front_index();
}

void Arduboy2Core::paintScreen(const uint8_t *image)
{
  paintScreen(const_cast<uint8_t *>(image), false);
}

void Arduboy2Core::paint8Pixels(uint8_t pixels)
{
  // Rarely used; keep a simple column cursor like the OLED version.
  uint8_t *back = lynx_back_fb();
  uint8_t ink = inkForRow(cursorY);

  for (uint8_t b = 0; b < 8; b++)
  {
    uint8_t r = (uint8_t)(cursorY + b);
    if (r >= WINDOW_H)
    {
      break;
    }
    uint8_t screenX = (uint8_t)(WINDOW_X + cursorX);
    if (screenX >= WINDOW_X + WINDOW_W)
    {
      break;
    }
    uint8_t *p = back + (((uint16_t)WINDOW_Y + r) * FB_STRIDE) + (screenX >> 1);
    uint8_t on = (pixels & (1U << b)) != 0;
    uint8_t v = alphaPixel(r, on);
    if (screenX & 1)
    {
      *p = (uint8_t)((*p & 0xF0) | v);
    }
    else
    {
      *p = (uint8_t)((*p & 0x0F) | (v << 4));
    }
  }

  cursorX++;
  if (cursorX >= WINDOW_W)
  {
    cursorX = 0;
    cursorY += 8;
    if (cursorY >= WINDOW_H)
    {
      cursorY = 0;
    }
  }
}

// ---------------------------------------------------------------------------
// Whole-window operations
// ---------------------------------------------------------------------------

void Arduboy2Core::blank()
{
  fillWindow(INK_OFF, 0, WINDOW_H);
}

void Arduboy2Core::allPixelsOn(bool on)
{
  if (on)
  {
    // Upstream: all pixels lit regardless of invert() state.
    fillWindow(INK_MONO, 0, WINDOW_H);
  }
  else
  {
    // Returning normal content happens on the next paintScreen().
  }
}

void Arduboy2Core::invert(bool inverse)
{
  invertFlag = inverse;
}

void Arduboy2Core::flipVertical(bool flipped)
{
  vFlip = flipped;
}

void Arduboy2Core::flipHorizontal(bool flipped)
{
  hFlip = flipped;
}

// ---------------------------------------------------------------------------
// Display control
// ---------------------------------------------------------------------------

void Arduboy2Core::displayOff()
{
  fillWindow(INK_OFF, 0, WINDOW_H);
}

void Arduboy2Core::displayOn()
{
  // Content returns with the next paintScreen()/display().
}

void Arduboy2Core::sendLCDCommand(uint8_t command)
{
  switch (command)
  {
    case OLED_ALL_PIXELS_ON:
      allPixelsOn(true);
      break;
    default:
      break;
  }
}

// ---------------------------------------------------------------------------
// Buttons
// ---------------------------------------------------------------------------

uint8_t Arduboy2Core::buttonsState()
{
  uint8_t k = lynx_keypad();
  uint8_t b = 0;

  // Directional pad physical bits (normal / left-handed):
  //   0x80 Up, 0x40 Down, 0x20 Left, 0x10 Right
  if (lynx_left_handed())
  {
    if (k & 0x80) b |= UP_BUTTON;
    if (k & 0x40) b |= DOWN_BUTTON;
    if (k & 0x20) b |= LEFT_BUTTON;
    if (k & 0x10) b |= RIGHT_BUTTON;
  }
  else
  {
    // Right-handed: the two pads are mirrored, so left/right switch.
    if (k & 0x80) b |= UP_BUTTON;
    if (k & 0x40) b |= DOWN_BUTTON;
    if (k & 0x20) b |= RIGHT_BUTTON;
    if (k & 0x10) b |= LEFT_BUTTON;
  }

  // B1 (inner) = A, B0 (outer) = B, active high.
  if (k & 0x04)
  {
    b |= A_BUTTON;
  }
  if (k & 0x02)
  {
    b |= B_BUTTON;
  }

  return b;
}

uint8_t Arduboy2Core::lynxOption1Key()
{
  return (lynx_keypad() & 0x08) ? 1 : 0;
}

uint8_t Arduboy2Core::lynxPauseKey()
{
  return lynx_pause_key();
}

void Arduboy2Core::lynxSetRichPalette(bool rich)
{
  richMode = rich;
}

bool Arduboy2Core::lynxRichPalette()
{
  return richMode;
}

// ---------------------------------------------------------------------------
// Boot / misc
// ---------------------------------------------------------------------------

void Arduboy2Core::boot()
{
  // lynx_video_init() already ran in main(); just reset renderer state.
  invertFlag = false;
  hFlip = false;
  vFlip = false;
  richMode = false;
  cursorX = 0;
  cursorY = 0;
  fillWindow(INK_OFF, 0, WINDOW_H);
}

void Arduboy2Core::safeMode()
{
  // Wait for all buttons to be released (upstream would check UP to enter the
  // bootloader; the Lynx has no re-flashable bootloader from the cartridge).
  while (buttonsState() != 0 || lynxOption1Key() || lynxPauseKey())
  {
  }
}

unsigned long Arduboy2Core::generateRandomSeed()
{
  // No ADC floating pin on the Lynx; mix the clock with a couple of
  // low-level reads that vary with timing.
  unsigned long s = micros();
  s ^= (unsigned long)lynx_keypad() << 16;
  s ^= (unsigned long)lynx_pause_key() << 8;
  delayMicroseconds(97);      // vary the timing a touch
  s ^= micros();
  s ^= s << 13;
  s ^= s >> 17;
  s ^= s << 5;
  return s;
}

void Arduboy2Core::delayShort(uint16_t ms)
{
  delay((unsigned long)ms);
}

void Arduboy2Core::exitToBootloader()
{
  // No bootloader on the Lynx; return to the caller.
}

void Arduboy2Core::idle()
{
  // No CPU sleep instruction on the 65C02; just let the timing produce some
  // slack. Keep it cheap so nextFrame() can call it freely.
}

// ---------------------------------------------------------------------------
// No-op / stubbed hardware control
// ---------------------------------------------------------------------------

void Arduboy2Core::LCDDataMode() {}
void Arduboy2Core::LCDCommandMode() {}
void Arduboy2Core::SPItransfer(uint8_t) {}
uint8_t Arduboy2Core::SPItransferAndRead(uint8_t) { return 0; }

void Arduboy2Core::setCPUSpeed8MHz() {}
void Arduboy2Core::bootSPI() {}
void Arduboy2Core::bootOLED() {}
void Arduboy2Core::bootPins() {}
void Arduboy2Core::bootPowerSaving() {}

void Arduboy2Core::setRGBled(uint8_t, uint8_t, uint8_t) {}
void Arduboy2Core::setRGBled(uint8_t, uint8_t) {}
void Arduboy2Core::freeRGBled() {}
void Arduboy2Core::digitalWriteRGB(uint8_t, uint8_t, uint8_t) {}
void Arduboy2Core::digitalWriteRGB(uint8_t, uint8_t) {}

#endif // !defined(__AVR__)