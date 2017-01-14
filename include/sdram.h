#ifndef SDRAM_included
#define SDRAM_included

#define SDRAM_BASE_ADDRESS 0xd0000000

void init_sdram(void);

// variable attribute to put global/static variables into SDRAM.
// SDRAM is initialized to zero, so don't use with initialized variables.

#define SDRAM_BANK(n) __attribute__((section(".sdram_bank_" #n)))

#define SDRAM_BANK_0 SDRAM_BANK(0)
#define SDRAM_BANK_1 SDRAM_BANK(1)
#define SDRAM_BANK_2 SDRAM_BANK(2)
#define SDRAM_BANK_3 SDRAM_BANK(3)

#endif /* !SDRAM_included */
