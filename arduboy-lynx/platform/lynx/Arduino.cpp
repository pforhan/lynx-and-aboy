/*
  lynx-and-aboy: Arduino API shim implementation.
*/

#include "Arduino.h"
#include "Lynx.h"

// ---------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------

unsigned long millis()
{
  return lynx_millis();
}

unsigned long micros()
{
  return lynx_micros();
}

void delay(unsigned long ms)
{
  unsigned long end = ms + millis();
  while (millis() < end)
  {
  }
}

void delayMicroseconds(unsigned int us)
{
  // Each loop iteration is ~4 cycles on the 65C02 (LDA/ADC/JNZ-style), i.e.
  // roughly 1us at the 4MHz Lynx clock.
  volatile uint16_t count = us;
  while (count != 0)
  {
    count--;
  }
}

void yield()
{
  // Nothing to cooperate with; games poll input themselves.
}

// ---------------------------------------------------------------------------
// Pins / digital I/O (no-ops; there are no external pins on a gamecart ROM)
// ---------------------------------------------------------------------------

void pinMode(uint8_t, uint8_t) {}
void digitalWrite(uint8_t, uint8_t) {}
int digitalRead(uint8_t) { return LOW; }
int analogRead(uint8_t) { return 0; }
void analogWrite(uint8_t, int) {}

// ---------------------------------------------------------------------------
// Random
// ---------------------------------------------------------------------------

static unsigned long randomNext = 1;

void randomSeed(unsigned long seed)
{
  randomNext = seed ? seed : 1;
}

long random(long howbig)
{
  // Simple xorshift; Anday steps through it. Good enough for a toy.
  randomNext ^= randomNext << 13;
  randomNext ^= randomNext >> 17;
  randomNext ^= randomNext << 5;
  return (long)(randomNext % (unsigned long)howbig);
}

long random(long howsmall, long howbig)
{
  if (howsmall >= howbig)
  {
    return howsmall;
  }
  return howsmall + random(howbig - howsmall);
}

// ---------------------------------------------------------------------------
// Math
// ---------------------------------------------------------------------------

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}