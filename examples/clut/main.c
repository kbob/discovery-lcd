#include <string.h>

#include "clock.h"
#include "lcd.h"
#include "lcd-pwm.h"
#include "sdram.h"
#include "systick.h"

#define MY_CLOCKS (rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ])
#define MY_SCREEN adafruit_p1596_lcd

#define L1PF al44
#define L1W 200
#define L1H 330
#define L1X ((800 - L1W) / 2)
#define L1Y ((480 - L1H) / 2)
#define L1NCOLOR 16

#define L2PF al88
#define L2H 256
#define L2W 256
#define L2X ((800 - L2W) / 2)
#define L2Y ((480 - L2H) / 2)
#define L2NCOLOR (6 * 64)
#define L2NCLU (L2NCOLOR + 256)

static L1PF layer1_pixel_buf[L1H][L1W] SDRAM_BANK_0;
static L2PF layer2_pixel_buf[L2H][L2W] SDRAM_BANK_1;
static lcd_clut16 layer1_clut;
static xrgb_888 layer2_clut_buf[L2NCLU];

static lcd_settings my_settings = {
    .bg_pixel = 0xFF7F007F,
    .layer1 = {
        .is_enabled = true,
        .uses_pixel_alpha = true,
        .uses_subjacent_alpha = true,
        .alpha = 0xFF,
        .position = { L1X, L1Y },
        .color_key_pixel = 0xFF00FF00,
        .pixels = {
            .pitch = sizeof *layer1_pixel_buf,
            .format = PF_AL44,
            .w = L1W,
            .h = L1H,
            .pixels = layer1_pixel_buf,
        },
        .clut = (lcd_clut *)layer1_clut,
    },
    .layer2 = {
        .is_enabled = true,
        .uses_pixel_alpha = true,
        .uses_subjacent_alpha = true,
        .alpha = 0xFF,
        .position = { L2X, L2Y },
        .pixels = {
            .pitch = sizeof *layer2_pixel_buf,
            .format = PF_AL88,
            .w = L2W,
            .h = L2H,
            .pixels = layer2_pixel_buf,
        },
        .clut = (lcd_clut *)layer2_clut_buf,
    },
};

static void draw_layer_1(void)
{
    for (size_t y = 0; y < L1H; y++) {
        for (size_t x = 0; x < L1W; x++) {
            uint32_t stripe = (L1H + x - y) >> 3;
            uint8_t color = stripe % 2 ? 0 : (stripe >> 1) % 15 + 1;
            layer1_pixel_buf[y][x] = 0xF0 | color;
        }
    }
}

static void draw_layer_2(void)
{
    for (size_t y = 0; y < L2H; y++)
        for (size_t x = 0; x < L2W; x++)
            layer2_pixel_buf[y][x] = y << 8 | x;
}

static void init_layer_1_clut(void)
{
    layer1_clut[ 0] = 0x000000;
    layer1_clut[ 1] = 0x004aff;
    layer1_clut[ 2] = 0x95ff00;
    layer1_clut[ 3] = 0xff00df;
    layer1_clut[ 4] = 0x00ffd4;
    layer1_clut[ 5] = 0xff8a00;
    layer1_clut[ 6] = 0x4000ff;
    layer1_clut[ 7] = 0x0bff00;
    layer1_clut[ 8] = 0xff0055;
    layer1_clut[ 9] = 0x00a0ff;
    layer1_clut[10] = 0xeaff00;
    layer1_clut[11] = 0xca00ff;
    layer1_clut[12] = 0x00ff7f;
    layer1_clut[13] = 0xff3500;
    layer1_clut[14] = 0x0016ff;
    layer1_clut[15] = 0x60ff00;
}

static void init_layer_2_clut(void)
{
    for (size_t i = 0; i < L2NCOLOR; i++) {
        uint32_t inc = (i % 64) << 2, high = 0xFF, dec = high - inc;
        uint32_t r = 0, g = 0, b = 0;
        switch (i / 64) {

        case 0:
            r = 0xFF;
            g = inc;
            break;

        case 1:
            r = dec;
            g = 0xFF;
            break;

        case 2:
            g = 0xFF;
            b = inc;
            break;

        case 3:
            g = dec;
            b = 0xFF;
            break;

        case 4:
            b = 0xFF;
            r = inc;
            break;

        case 5:
            b = dec;
            r = high;
            break;
        }
        layer2_clut_buf[i] = r << 16 | g << 8 | b << 0;
    }
    for (size_t i = L2NCOLOR; i < L2NCLU; i++)
        layer2_clut_buf[i] = layer2_clut_buf[i - L2NCOLOR];
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

static lcd_settings *frame_callback(lcd_settings *s)
{
    static uint32_t l1_clut_rotor;
    l1_clut_rotor++;
    if (l1_clut_rotor == 30) {
        l1_clut_rotor = 0;
        xrgb_888 c1 = layer1_clut[1];
        memmove(layer1_clut + 1, layer1_clut + 2, 14 * sizeof (layer1_clut[0]));
        layer1_clut[15] = c1;
    }

    static uint32_t l2_clut_rotor;
    l2_clut_rotor = (l2_clut_rotor + 1) % L2NCOLOR;
    s->layer2.clut = (lcd_clut *)(layer2_clut_buf + l2_clut_rotor);

    return s;
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
    init_layer_1_clut();
    init_layer_2_clut();
    lcd_set_frame_callback(frame_callback);
    lcd_load_settings(&my_settings, false);
    while (1) {
        fade_in_LCD();
    }
}
