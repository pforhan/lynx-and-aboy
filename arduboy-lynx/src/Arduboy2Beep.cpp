/**
 * @file Arduboy2Beep.cpp
 * \brief
 * Classes to generate simple square wave tones.
 *
 * LYNX PORT: BeepPin1 is mapped to Lynx audio channel A (index 0), BeepPin2
 * to channel B (index 1). Each tone is a polymorphic-counter square wave
 * produced entirely by Mikey hardware (see NOTES.md).
 *
 * Period conversion:
 *   BeepPin1:  count = 1e6/freq - 1  ->  square period = 1e6/(count+1) us
 *   BeepPin2:  count = 62500/freq -1 ->  square period = 16*(count+1) us
 */

#include <Arduino.h>
#include "Arduboy2Beep.h"
#include "Lynx.h"

#if !defined(__AVR__)
// Lynx implementation of BeepPin1/BeepPin2 (see also Arduboy2Audio.cpp).

// ---------------------------------------------------------------------------
// Speaker pin 1 -> Lynx audio channel A (0)
// ---------------------------------------------------------------------------

uint8_t BeepPin1::duration = 0;

void BeepPin1::begin()
{
  // Nothing to do: Lynx audio requires no GPIO setup.
}

void BeepPin1::tone(uint16_t count)
{
  tone(count, 0);
}

void BeepPin1::tone(uint16_t count, uint8_t dur)
{
  duration = dur;
  // BeepPin1: freq = 1e6/(count+1) Hz, so the square-wave period in us is
  // exactly (count+1). lynx_tone_square() halves it to program the audio
  // timer's period (the polynomial-counter output toggles every audio clock,
  // so a full square cycle is two audio-clock periods).
  lynx_tone_square(0, (uint32_t)count + 1);
}

void BeepPin1::timer()
{
  if (duration && (--duration == 0))
  {
    lynx_tone_stop(0);
  }
}

void BeepPin1::noTone()
{
  duration = 0;
  lynx_tone_stop(0);
}

// ---------------------------------------------------------------------------
// Speaker pin 2 -> Lynx audio channel B (1)
// ---------------------------------------------------------------------------

uint8_t BeepPin2::duration = 0;

void BeepPin2::begin()
{
}

void BeepPin2::tone(uint16_t count)
{
  tone(count, 0);
}

void BeepPin2::tone(uint16_t count, uint8_t dur)
{
  duration = dur;
  // BeepPin2: freq = 62500/(count+1) Hz, so the square-wave period in us is
  // 1e6/freq = 16*(count+1).
  lynx_tone_square(1, ((uint32_t)count + 1) * 16);
}

void BeepPin2::timer()
{
  if (duration && (--duration == 0))
  {
    lynx_tone_stop(1);
  }
}

void BeepPin2::noTone()
{
  duration = 0;
  lynx_tone_stop(1);
}

#endif // !defined(__AVR__)