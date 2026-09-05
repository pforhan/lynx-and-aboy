/*
  lynx-and-aboy: Arduino API shim for the Atari Lynx (llvm-mos).

  Provides just enough of the Arduino vocabulary for Arduboy2-compatible
  sketches to compile and run on the Lynx. Functionality is either real
  (millis/micros/delay/Print/EEPROM backed by Lynx hardware) or a no-op
  where the concept doesn't exist on the Lynx (pins, analog, etc.).
*/

#ifndef LYNYN_ARDUINO_H
#define LYNYN_ARDUINO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "avr/pgmspace.h"

// ---------------------------------------------------------------------------
// Sketches / Arduino core
// ---------------------------------------------------------------------------

#define ARDUINO 10819
#define F_CPU 16000000UL

typedef uint8_t byte;
typedef uint16_t word;

// void setup(); void loop();  -- supplied by the sketch

// ---------------------------------------------------------------------------
// Basic utilities
// ---------------------------------------------------------------------------

#define _BV(b) (1UL << (b))
#define bit(b) (1UL << (b))
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) \
  ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

#define lowByte(w) ((uint8_t)((w)&0xFF))
#define highByte(w) ((uint8_t)(((w) >> 8) & 0xFF))

#define abs(x) ((x) < 0 ? -(x) : (x))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define constrain(amt, lo, hi) ((amt) < (lo) ? (lo) : ((amt) > (hi) ? (hi) : (amt)))
#define sq(x) ((x) * (x))

long map(long x, long in_min, long in_max, long out_min, long out_max);

#define PI 3.1415926535897932384626433832795

// ---------------------------------------------------------------------------
// Pins / digital I/O (no-ops on Lynx; kept for compile compatibility)
// ---------------------------------------------------------------------------

#define HIGH 0x1
#define LOW 0x0
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

#define LED_BUILTIN 13

// On the real Arduboy these poke the TX board LED for the CPU-load indicator.
#define TXLED0 ((void)0)
#define TXLED1 ((void)0)

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
int digitalRead(uint8_t pin);
int analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int value);

// ---------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------

unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);
void yield();

// ---------------------------------------------------------------------------
// Random
// ---------------------------------------------------------------------------

void randomSeed(unsigned long seed);
long random(long howbig);
long random(long howsmall, long howbig);

// ---------------------------------------------------------------------------
// Misc Arduino-isms
// ---------------------------------------------------------------------------

#define F(string_literal) (reinterpret_cast<const __FlashStringHelper *>(string_literal))

class __FlashStringHelper;

// Suppress -Wunused for pure-compat helpers if a build uses lots of them.
void setup();
void loop();
int main();

#endif // LYNYN_ARDUINO_H