/*
  lynx-and-aboy: RAM-backed EEPROM shim.
*/

#include "EEPROM.h"

#include <string.h>

static uint8_t eepromData[EEPROM_LENGTH];

uint8_t EEPROMClass::read(int address)
{
  if (address < 0 || address >= EEPROM_LENGTH)
  {
    return 0;
  }
  return eepromData[address];
}

void EEPROMClass::write(int address, uint8_t value)
{
  if (address < 0 || address >= EEPROM_LENGTH)
  {
    return;
  }
  eepromData[address] = value;
}

void EEPROMClass::update(int address, uint8_t value)
{
  if (address < 0 || address >= EEPROM_LENGTH)
  {
    return;
  }
  if (eepromData[address] != value)
  {
    eepromData[address] = value;
  }
}

uint8_t *EEPROMClass::getDataPtr()
{
  return eepromData;
}

const uint8_t *EEPROMClass::getConstDataPtr()
{
  return eepromData;
}

size_t EEPROMClass::length()
{
  return EEPROM_LENGTH;
}

EEPROMClass EEPROM;