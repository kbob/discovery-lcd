#ifndef SYSTICK_included
#define SYSTICK_included

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef void systick_handler(uint32_t millis);

    extern volatile uint32_t system_millis;

    extern void systick_setup(uint32_t cpu_freq);

    extern void register_systick_handler(systick_handler *);

#ifdef __cplusplus
}
#endif

#endif /* !SYSTICK_included */
