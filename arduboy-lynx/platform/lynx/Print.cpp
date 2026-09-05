/*
  lynx-and-aboy: minimal Print implementation.
*/

#include "Print.h"

#include <string.h>

#include "Arduino.h"

size_t Print::write(const char *str)
{
  size_t n = 0;
  while (*str)
  {
    n += write((uint8_t)*str++);
  }
  return n;
}

size_t Print::printNumber(unsigned long n, uint8_t base)
{
  char buf[8 * sizeof(long) + 1];
  char *str = &buf[sizeof(buf) - 1];
  char *end = str;

  do
  {
    char c = (char)(n % base);
    n /= base;
    *str-- = c < 10 ? c + '0' : c - 10 + 'A';
  } while (n != 0);

  (void)end;
  return write(str + 1);
}

size_t Print::printFloat(double number, uint8_t digits)
{
  size_t n = 0;

  if (number < 0.0)
  {
    n += print('-');
    number = -number;
  }

  // Round to the requested precision so the printed digits reflect .5 etc.
  double rounding = 0.5;
  for (uint8_t i = 0; i < digits; ++i)
  {
    rounding *= 10.0;
  }
  number += rounding;

  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;

  n += printNumber(int_part, 10);

  if (digits > 0)
  {
    n += print('.');
    while (digits-- > 0)
    {
      remainder *= 10.0;
      int toPrint = (int)remainder;
      n += print((char)('0' + toPrint));
      remainder -= toPrint;
    }
  }

  return n;
}

size_t Print::print(const char str[]) { return write(str); }
size_t Print::print(char c) { return write((uint8_t)c); }
size_t Print::print(unsigned char b, int base) { return printNumber(b, base); }
size_t Print::print(int n, int base)
{
  if (base == 10 && n < 0)
  {
    size_t r = write((uint8_t)'-');
    return r + printNumber((unsigned long)(-(long)n), base);
  }
  return printNumber(n, base);
}
size_t Print::print(unsigned int n, int base) { return printNumber(n, base); }
size_t Print::print(long n, int base)
{
  if (base == 10 && n < 0)
  {
    size_t r = write((uint8_t)'-');
    return r + printNumber((unsigned long)(-n), base);
  }
  return printNumber(n, base);
}
size_t Print::print(unsigned long n, int base) { return printNumber(n, base); }
size_t Print::print(double n, int digits) { return printFloat(n, digits); }
size_t Print::print(const __FlashStringHelper *f) { return write((const char *)f); }
size_t Print::print(const void *ptr, int base) { return printNumber((unsigned long)ptr, base); }

size_t Print::println() { return write('\n'); }

#define DEF_PRINTLN(TYPE, EXPR)                       \
  size_t Print::println(TYPE v, int base)             \
  {                                                   \
    size_t n = print(v, base);                        \
    n += println();                                   \
    return n;                                         \
  }

size_t Print::println(const char c[]) { return print(c) + println(); }
size_t Print::println(char c) { return print(c) + println(); }
DEF_PRINTLN(unsigned char, b)
DEF_PRINTLN(int, n)
DEF_PRINTLN(unsigned int, n)
DEF_PRINTLN(long, n)
DEF_PRINTLN(unsigned long, n)
DEF_PRINTLN(double, n)
size_t Print::println(const __FlashStringHelper *f) { return print(f) + println(); }
size_t Print::println(const void *ptr, int base) { return print(ptr, base) + println(); }