#include "clock.h"

void init_clocks(const struct rcc_clock_scale *clock_scale)
{
    rcc_clock_setup_hse_3v3(clock_scale);
}
