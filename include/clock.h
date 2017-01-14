#ifndef CLOCK_included
#define CLOCK_included

#include <libopencm3/stm32/rcc.h>

extern void init_clocks(const struct rcc_clock_scale *);

#endif /* !CLOCK_included */
