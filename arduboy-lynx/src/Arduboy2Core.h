/**
 * @file Arduboy2Core.h
 * \brief
 * The Arduboy2Core class for Arduboy hardware initialization and control.
 *
 * LYNX PORT: This is the Atari Lynx implementation. The public API matches
 * upstream Arduboy2 (v6.0.0) exactly, but every method talks to Lynx
 * hardware (Mikey/Suzy) instead of the AVR OLED/SPI peripherals.
 */

#ifndef ARDUBOY2_CORE_H
#define ARDUBOY2_CORE_H

#include <Arduino.h>
#include <avr/power.h>
#include <avr/sleep.h>

#define RGB_ON LOW   /**< Set an RGB LED on using digitalWriteRGB() */
#define RGB_OFF HIGH /**< Set an RGB LED off using digitalWriteRGB() */

// ----- Button values (identical masks to the production Arduboy) -----
#define LEFT_BUTTON _BV(5)  /**< Left button value for bitmask functions */
#define RIGHT_BUTTON _BV(6) /**< Right button value for bitmask functions */
#define UP_BUTTON _BV(7)    /**< Up button value for bitmask functions */
#define DOWN_BUTTON _BV(4)  /**< Down button value for bitmask functions */
#define A_BUTTON _BV(3)     /**< A button value for bitmask functions */
#define B_BUTTON _BV(2)     /**< B button value for bitmask functions */

// ----- RGB LED pin ids (no physical LED on the Lynx; kept for compile) -----
#define RED_LED 10
#define GREEN_LED 11
#define BLUE_LED 9

// ----- The Lynx keyboard is active-high for pressed -----
#define KEY_PAD_MASK 0xF0    // physical Up/Down/Left/Right live in bits 7..4

// ----- Display size -----
#define WIDTH 128 /**< Display width in pixels */
#define HEIGHT 64 /**< Display height in pixels */

// Simulate the "all pixels on" display command used by Arduboy2.cpp.
#define OLED_ALL_PIXELS_ON 0xA5

/**
 * \brief
 * Lower level functions generally dealing directly with the hardware.
 *
 * \details
 * This class is inherited by Arduboy2Base and thus also Arduboy2, so wouldn't
 * normally be used directly by a sketch. The public surface is identical to
 * upstream Arduboy2Core; the Lynx additions are grouped at the bottom.
 */
class Arduboy2Core
{
  public:

    // PPU owns the frame pacing; idle() simply yields the CPU for a millisecond.
    static void idle();

    // No-op command/data mode & SPI: there is no external OLED controller.
    static void LCDDataMode();
    static void LCDCommandMode();
    static void SPItransfer(uint8_t data);
    static uint8_t SPItransferAndRead(uint8_t data);

    // The Lynx screen has no "off" state, but we can blank it cheaply.
    static void displayOff();
    static void displayOn();

    static constexpr uint8_t width() { return WIDTH; }
    static constexpr uint8_t height() { return HEIGHT; }

    // Human-oriented button masks, identical to upstream.
    static uint8_t buttonsState();

    // Direct draw to the Lynx framebuffer (game-visible window), 1bpp-src.
    static void paint8Pixels(uint8_t pixels);
    static void paintScreen(const uint8_t *image);
    static void paintScreen(uint8_t image[], bool clear = false);
    static void blank();
    static void invert(bool inverse);
    static void allPixelsOn(bool on);
    static void flipVertical(bool flipped);
    static void flipHorizontal(bool flipped);

    static void sendLCDCommand(uint8_t command);

    // No RGB LED on the Lynx; stubs.
    static void setRGBled(uint8_t red, uint8_t green, uint8_t blue);
    static void setRGBled(uint8_t color, uint8_t val);
    static void freeRGBled();
    static void digitalWriteRGB(uint8_t red, uint8_t green, uint8_t blue);
    static void digitalWriteRGB(uint8_t color, uint8_t val);

    static void boot();
    static void safeMode();
    static unsigned long generateRandomSeed();
    static void delayShort(uint16_t ms) __attribute__((noinline));
    static void exitToBootloader();

    // ---------------------------------------------------------------------
    // Lynx extras (no-op/absent on a real Arduboy; guard with __LYNX__).
    // ---------------------------------------------------------------------

    /** Option 1 key as a button-state mask bit (physical keypad bit 3). */
    static uint8_t lynxOption1Key();

    /** Pause switch state (1 = pressed). */
    static uint8_t lynxPauseKey();

    /** Use the Lynx's 4bpp color, or stay in the Arduboy's monochrome look. */
    static void lynxSetRichPalette(bool rich);

    /** Current rich-palette mode. */
    static bool lynxRichPalette();

  protected:

    static void setCPUSpeed8MHz();
    static void bootSPI();
    static void bootOLED();
    static void bootPins();
    static void bootPowerSaving();
};

#endif // ARDUBOY2_CORE_H