#ifndef LYNYN_IO_H
#define LYNYN_IO_H

/* Minimal AVR <avr/io.h> stand-in. Most sketches never touch it directly on
   the Arduboy (they use Arduboy2), so only dummy pin/port macros are offered
   for compile compatibility. */

#include <stdint.h>

#define HIGH 0x1
#define LOW 0x0

#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

#endif