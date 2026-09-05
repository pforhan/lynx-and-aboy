#ifndef LYNYN_SLEEP_H
#define LYNYN_SLEEP_H

/* AVR sleep-mode stubs. The Lynx has no CPU instruction to "idle", the port
   busy-polls instead (see Arduboy2Core::idle()). */

#define sleep_enable() ((void)0)
#define sleep_disable() ((void)0)
#define sleep_mode() ((void)0)
#define sleep_cpu() ((void)0)

#define SLEEP_MODE_IDLE 0
#define set_sleep_mode(mode) ((void)0)

#endif