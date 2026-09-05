/*
  lynx-and-aboy demo.

  A minimal Arduboy sketch that also exercises the Lynx-only extras:

    - A button      : play a bounce blip
    - UP/DOWN       : invert / restore display
    - Option 1      : toggle the Lynx rich color palette (Lynx only)
    - Pause         : freeze with an overlay until Pause is released

  Builds unmodified for a real Arduboy too; the `#ifdef __LYNX__` blocks are
  inert there because the toolchain defines __LYNX__ only on the Lynx.
*/

#include <Arduboy2.h>

Arduboy2 arduboy;
BeepPin1 beep;

int ballX = 30;
int ballY = 20;
int ballVX = 1;
int ballVY = -1;

#ifdef __LYNX__
static uint8_t prevOption1 = 0;
static bool paused = false;
#endif

void drawText()
{
  arduboy.setCursor(16, 6);
  arduboy.print(F("LYNX AND A BOY"));
}

void drawPauseOverlay()
{
  arduboy.fillRect(0, HEIGHT / 2 - 3, WIDTH, 6, BLACK);
  arduboy.setCursor(0, HEIGHT / 2 - 2);
  arduboy.print(F("PAUSED"));
}

void setup()
{
  arduboy.begin();
  arduboy.setFrameRate(30);
  beep.begin();
  drawText();
}

void loop()
{
  if (!arduboy.nextFrame())
  {
    return;
  }

  arduboy.pollButtons();
  beep.timer();

#ifdef __LYNX__
  // Option 1: toggle rich color mode (edge-triggered).
  uint8_t nowOption1 = Arduboy2Core::lynxOption1Key();
  if (nowOption1 && !prevOption1)
  {
    Arduboy2Core::lynxSetRichPalette(!Arduboy2Core::lynxRichPalette());
  }
  prevOption1 = nowOption1;

  // Pause: freeze all gameplay while the switch is held.
  paused = Arduboy2Core::lynxPauseKey() != 0;
#endif

#ifdef __LYNX__
  if (!paused)
  {
#endif
    ballX += ballVX;
    ballY += ballVY;

    if (ballX < 4 || ballX > WIDTH - 5)
    {
      ballVX = -ballVX;
      beep.tone(beep.freq(1400), 4);
    }

    if (ballY < 2 || ballY > HEIGHT - 3)
    {
      ballVY = -ballVY;
      beep.tone(beep.freq(2000), 4);
    }

    if (arduboy.pressed(UP_BUTTON))
    {
      arduboy.invert(true);
    }
    else if (arduboy.pressed(DOWN_BUTTON))
    {
      arduboy.invert(false);
    }

    if (arduboy.justPressed(A_BUTTON))
    {
      beep.tone(beep.freq(880), 8);
    }
#ifdef __LYNX__
  }
#endif

  arduboy.clear();
  drawText();
  arduboy.fillCircle(ballX, ballY, 3);

#ifdef __LYNX__
  if (paused)
  {
    drawPauseOverlay();
  }
#endif

  arduboy.display();
}