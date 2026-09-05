#ifndef LYNYN_POWER_H
#define LYNYN_POWER_H

/* AVR power-management stubs. The Lynx does not idle peripheral clocks. */

#define power_all_disable() ((void)0)
#define power_all_enable() ((void)0)
#define power_adc_disable() ((void)0)
#define power_timer0_disable() ((void)0)
#define power_timer1_disable() ((void)0)
#define power_timer2_disable() ((void)0)
#define power_timer3_disable() ((void)0)
#define power_timer4_disable() ((void)0)
#define power_spi_disable() ((void)0)
#define power_usart0_disable() ((void)0)
#define power_usb_disable() ((void)0)

#endif