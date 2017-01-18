#ifndef CLOCK_included
#define CLOCK_included

#include <libopencm3/stm32/rcc.h>

#ifdef __cplusplus
extern "C" {
#endif

    extern void init_clocks(const struct rcc_clock_scale *);

#ifdef __cplusplus
}
#endif


#endif /* !CLOCK_included */
