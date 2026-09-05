/**
 * @file Arduboy2Audio.cpp
 * \brief
 * The Arduboy2Audio class for speaker and sound control.
 *
 * LYNX PORT: "mute" silences the Lynx audio channels (volume 0). Tones
 * continue to be generated in the background so unmute is instant.
 */

#include "Arduboy2.h"
#include "Lynx.h"

#if !defined(__AVR__)
// Lynx implementation of Arduboy2Audio (see also Arduboy2Beep.cpp).

bool Arduboy2Audio::audio_enabled = false;

void Arduboy2Audio::on()
{
  audio_enabled = true;
  lynx_audio_mute(false);
}

void Arduboy2Audio::off()
{
  audio_enabled = false;
  lynx_audio_mute(true);
}

void Arduboy2Audio::toggle()
{
  if (audio_enabled)
  {
    off();
  }
  else
  {
    on();
  }
}

void Arduboy2Audio::saveOnOff()
{
  EEPROM.update(Arduboy2Base::eepromAudioOnOff, audio_enabled);
}

void Arduboy2Audio::begin()
{
  if (EEPROM.read(Arduboy2Base::eepromAudioOnOff))
  {
    on();
  }
  else
  {
    off();
  }
}

bool Arduboy2Audio::enabled()
{
  return audio_enabled;
}

#endif // !defined(__AVR__)