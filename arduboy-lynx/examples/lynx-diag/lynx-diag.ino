/*
  lynx-diag: hardware/video pipeline diagnostic.

  Fills BOTH framebuffers with bright pixels right after video init, then
  spins forever. No paintScreen, no VBL wait, no DISPADR swap, no game code.

  If the panel shows a bright solid color -> DMA, color map, and DISPADR are
  all working, and the bug is downstream (paint/swap/game logic).
  If it stays black -> the video init / color path itself is broken.
*/

#include "Arduino.h"
#include "Lynx.h"
#include <string.h>

void setup()
{
  uint8_t *p = lynx_front_fb();
  memset(p, 0x0F, (size_t)160 * 102 / 2);
  p = lynx_back_fb();
  memset(p, 0x0F, (size_t)160 * 102 / 2);
}

void loop()
{
}