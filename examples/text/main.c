#include "clock.h"
#include "lcd.h"
#include "lcd-pwm.h"
#include "sdram.h"
#include "systick.h"
#include "text.h"

#define MY_CLOCKS (rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ])
#define MY_SCREEN adafruit_p1596_lcd

#define L1PF rgb565
#define L1X   0
#define L1Y   0
#define L1H 480
#define L1W 800

L1PF layer1_pixel_buf[L1H][L1W] SDRAM_BANK_0;

lcd_settings my_settings = {
    .bg_pixel = 0xFFFF0000,
    .layer1 = {
        .is_enabled = true,
        .uses_pixel_alpha = true,
        .uses_subjacent_alpha = true,
        .default_pixel = 0x00000000,
        .alpha = 0xFF,
        .position = { L1X, L1Y},
        .pixels = {
            .pitch = sizeof *layer1_pixel_buf,
            .format = PF_RGB565,
            .w = L1W,
            .h = L1H,
            .pixels = layer1_pixel_buf,
        },
    },
};

static void draw_layer_1(void)
{
    for (size_t y = 0; y < L1H; y++)
        for (size_t x = 0; x < L1W; x++)
            layer1_pixel_buf[y][x] = 0x1047;

    render_text((uint16_t *)*layer1_pixel_buf, L1W, L1H);
}

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
    init_sdram();
    init_text();

    draw_layer_1();
    lcd_load_settings(&my_settings, false);
    while (1) {
        fade_in_LCD();
    }
}
