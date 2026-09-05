/*
  lynx-and-aboy: sketch entry point for the Atari Lynx.

  The llvm-mos crt0 calls main() with the hardware already reset. We set up
  the Lynx video + clock, then run an Arduino-style setup()/loop() forever.
*/

#include "Arduino.h"
#include "Lynx.h"

// Video frame buffers (4bpp, 160 x 102 = 8160 bytes each). 16-bit addresses
// on the 65C02 keep each buffer fully inside one 256-byte page group; DISPADR
// wraps at 256 so the buffers must start at 256-byte boundaries when using
// the byte-at-a-time registers. We use two alternating buffers for double
// buffering.
//
// The linker script places .bss at 0x200; declare buffers with 256-byte
// alignment so DISPADR page handling is trivial.

#define BUFSIZE 8160

uint8_t fb[2][BUFSIZE] __attribute__((aligned(256)));
static uint8_t fbIndex;

int main()
{
  lynx_clock_init();

  // Point the DMA at the first buffer, then kick off the display chain.
  lynx_video_init(fb[0]);

  setup();

  for (;;)
  {
    loop();
  }

  return 0;
}

// The visible frame alternates; the renderer writes to the other buffer.
uint8_t *lynx_back_fb(void)
{
  return &fb[fbIndex ^ 1][0];
}

uint8_t *lynx_front_fb(void)
{
  return &fb[fbIndex][0];
}

void lynx_front_index(void)
{
  fbIndex ^= 1;
}