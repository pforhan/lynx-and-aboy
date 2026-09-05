/*
  lynx-and-aboy: AVR pointer/storage compatibility stubs.

  On the Lynx the whole binary lives in RAM, so "program memory" is just
  regular memory. These macros let the (verbatim) Arduboy2 sources compile
  unchanged.
*/

#ifndef LYNYN_PGMSPACE_H
#define LYNYN_PGMSPACE_H

#include <stdint.h>
#include <stddef.h>

#define PROGMEM

#define PSTR(s) (s)

#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#define pgm_read_word(addr) (*(const uint16_t *)(addr))
#define pgm_read_dword(addr) (*(const uint32_t *)(addr))
#define pgm_read_float(addr) (*(const float *)(addr))
#define pgm_read_ptr(addr) (*(const void *const *)(addr))

#define pgm_read_byte_near(addr) pgm_read_byte(addr)
#define pgm_read_word_near(addr) pgm_read_word(addr)
#define pgm_read_byte_far(addr) pgm_read_byte(addr)
#define pgm_read_word_far(addr) pgm_read_word(addr)

#define memcpy_P(dest, src, n) memcpy((dest), (src), (n))
#define strlen_P(s) strlen((const char *)(s))

#endif