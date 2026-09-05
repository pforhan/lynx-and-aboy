#ifndef LYNYN_WDT_H
#define LYNYN_WDT_H

/* AVR watchdog stubs. There is no watchdog on the Lynx. */

#define wdt_disable() ((void)0)
#define wdt_enable(timeout) ((void)0)
#define wdt_reset() ((void)0)
#define wdt_enable_reset_mode() ((void)0)
#define WDTO_15MS 0
#define WDTO_60MS 1
#define WDTO_120MS 2
#define WDTO_250MS 3
#define WDTO_500MS 4
#define WDTO_1S 5
#define WDTO_2S 6
#define WDTO_4S 7
#define WDTO_8S 8

#endif