/*
  lynx-and-aboy: minimal Arduino Print class. The full Arduino Print is an
  interface over a virtual write(uint8_t). We provide just that, plus the
  print/println overloads that Arduboy2 uses (`print`/`println` of integer
  types, strings and characters).
*/

#ifndef LYNYN_PRINT_H
#define LYNYN_PRINT_H

#include <stdint.h>
#include <stddef.h>

class __FlashStringHelper;

class Print
{
public:
  Print() {}

  virtual size_t write(uint8_t) = 0;

  size_t print(const char[]);
  size_t print(char);
  size_t print(unsigned char, int base = 10);
  size_t print(int, int base = 10);
  size_t print(unsigned int, int base = 10);
  size_t print(long, int base = 10);
  size_t print(unsigned long, int base = 10);
  size_t print(double, int digits = 2);
  size_t print(const __FlashStringHelper *);
  size_t print(const void *, int base = 16);

  size_t println(const char[]);
  size_t println(char);
  size_t println(unsigned char, int base = 10);
  size_t println(int, int base = 10);
  size_t println(unsigned int, int base = 10);
  size_t println(long, int base = 10);
  size_t println(unsigned long, int base = 10);
  size_t println(double, int digits = 2);
  size_t println(const __FlashStringHelper *);
  size_t println(const void *, int base = 16);
  size_t println();

protected:
  size_t write(const char *str);
  size_t printNumber(unsigned long n, uint8_t base);
  size_t printFloat(double number, uint8_t digits);
};

#endif // LYNYN_PRINT_H