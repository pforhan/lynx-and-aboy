#ifndef LYNYN_INTERRUPT_H
#define LYNYN_INTERRUPT_H

/* Interrupt stubs. The port uses no interrupts; everything is busy-polled. */

#define sei() ((void)0)
#define cli() ((void)0)
#define noInterrupts() ((void)0)
#define interrupts() ((void)0)
#define ISR(vector) void vector(void)

#define SREG 0
#define TIMSK0 0
#define TIMSK1 0
#define TIMSK3 0
#define TIMSK4 0

#endif