#include "clock.h"
#include "lcd.h"
#include "lcd-pwm.h"
#include "sdram.h"
#include "systick.h"

#define MY_CLOCKS (rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ])
#define MY_SCREEN adafruit_p1596_lcd

#define L1PF rgba4444
#define L1H 100
#define L1W 100

#define L2PF al44
#define L2H 200
#define L2W 128

L1PF layer1_buffer[L1H][L1W] SDRAM_BANK_0;
L2PF layer2_buffer[L2H][L2W] SDRAM_BANK_2;

lcd_settings my_settings = {
    .bg_pixel = 0xFFFF0000,
    .layer1 = {
        .is_enabled = true,
        .uses_pixel_alpha = true,
        .uses_subjacent_alpha = true,
        .default_pixel = 0x00000000,
        .alpha = 0xFF,
        .position = { 100, 100 },
        .pixels = {
            .pitch = sizeof *layer1_buffer,
            .format = PF_ARGB4444,
            .w = L1W,
            .h = L1H,
            .pixels = layer1_buffer,
        },
    },
    .layer2 = {
        .is_enabled = true,
        .uses_pixel_alpha = true,
        .uses_subjacent_alpha = true,
        .default_pixel = 0x00000000,
        .alpha = 0xFF,
        .position = { 150, 150 },
        .pixels = {
            .pitch = sizeof *layer2_buffer,
            .format = PF_AL44,
            .w = L2W,
            .h = L2H,
            .pixels = layer2_buffer,
        },
    },
};

static void draw_layer_1(void)
{
    for (size_t y = 0; y < L1H; y++)
        for (size_t x = 0; x < L1W; x++)
            layer1_buffer[y][x] = ((L1H + x - y) >> 3) & 1 ? 0xF00F : 0x50F0;
}

static void draw_layer_2(void)
{
    for (size_t y = 0; y < L2H; y++)
        for (size_t x = 0; x < L2W; x++)
            layer2_buffer[y][x] = y >> 4 << 4 | x >> 4;
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

    draw_layer_1();
    draw_layer_2();
    lcd_load_settings(&my_settings, false);
    while (1) {
        fade_in_LCD();
    }
}
