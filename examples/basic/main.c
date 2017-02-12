#include "clock.h"
#include "lcd.h"
#include "lcd-pwm.h"
#include "systick.h"

#define MY_CLOCKS (rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ])
#define MY_SCREEN adafruit_p1596_lcd

static lcd_settings my_settings = {
    .bg_pixel = 0xFF0000,
};

int main(void)
{
    init_clocks(&MY_CLOCKS);
    systick_setup(MY_CLOCKS.ahb_frequency);
    lcd_pwm_setup();
    init_lcd(&MY_SCREEN);

    lcd_load_settings(&my_settings, false);
    lcd_fade(0, 65535, 0, 2000);
    while (1)
        continue;
}
