#include "clock.h"
#include "lcd.h"
#include "lcd-pwm.h"
#include "systick.h"

#define MY_CLOCKS (rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ])
#define MY_SCREEN adafruit_p1596_lcd

lcd_settings my_settings = {
    .bg_pixel = 0xFFFF0000,
};

static void fade_in_LCD(void)
{
    static bool done;
    if (done)
        return;
    uint32_t now = system_millis;
    static uint32_t t0;
    if (!t0) {
        t0 = now + 1;
        return;
    }
    uint32_t b0 = (now - t0);
    uint32_t b1 = b0 * (b0 + 1) >> 5;
    if (b1 > 65535) {
        b1 = 65535;
        done = true;
    }
    lcd_pwm_set_brightness(b1);
}

int main(void)
{
    init_clocks(&MY_CLOCKS);
    systick_setup(MY_CLOCKS.ahb_frequency);
    lcd_pwm_setup();
    init_lcd(&MY_SCREEN);

    lcd_load_settings(&my_settings, false);
    while (1) {
        fade_in_LCD();
    }
}
