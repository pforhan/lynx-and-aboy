/*
  lynx-and-aboy: EEPROM API shim.

  The Lynx has no EEPROM; the 1KiB (1024 byte) save space is provided as a
  region of RAM. It works for the lifetime of the current run and lets any
  EEPROM.save()/load() game logic execute unchanged.
*/

#ifndef LYNYN_EEPROM_H
#define LYNYN_EEPROM_H

#include <stdint.h>
#include <stddef.h>

#define EEPROM_LENGTH 1024

class EEPROMClass
{
public:
  uint8_t read(int address);
  void write(int address, uint8_t value);

  void update(int address, uint8_t value);

  template <typename T>
  T &get(int address, T &value)
  {
    return value;   // unsupported; kept for compile compatibility
  }

  template <typename T>
  const T &put(int address, const T &value)
  {
    return value;   // unsupported
  }

  uint8_t *getDataPtr();
  const uint8_t *getConstDataPtr();
  size_t length();
};

extern EEPROMClass EEPROM;

#endif // LYNYN_EEPROM_H