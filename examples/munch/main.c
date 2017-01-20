#include <stdlib.h>

#include "clock.h"
#include "lcd.h"
#include "lcd-pwm.h"
#include "sdram.h"
#include "systick.h"

#define MY_CLOCKS (rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ])
#define MY_SCREEN adafruit_p1596_lcd

#define L1PF l8
#define L1H 1024
#define L1W 1024
#define L1X ((800 - L1W) / 2)
#define L1Y ((480 - L1H) / 2)

#define L2PF al88
#define L2H 256
#define L2W 256

static L1PF layer1_pixel_buf[L1H][L1W] SDRAM_BANK_0;
static L2PF layer2_pixel_buf[L2H][L2W] SDRAM_BANK_2;
static xrgb_888 layer1_clut_buf[768];
static xrgb_888 layer2_clut_buf[768];

static uint8_t bit_reverse[256];

static uint8_t reverse_bits(uint8_t n)
{
    uint8_t a = n << 1 & 0b10101010 | n >> 1 & 0b01010101;
    uint8_t b = a << 2 & 0b11001100 | a >> 2 & 0b00110011;
    uint8_t c = b << 4 & 0b11110000 | b >> 4 & 0b00110011;
    return c;
}

static void init_bitrev(void)
{
    for (size_t i = 0; i < 256; i++)
        bit_reverse[i] = reverse_bits(i);
}

static lcd_settings my_settings = {
    .bg_pixel = 0xFF000000,
    .layer1 = {
        .is_enabled = true,
        .uses_pixel_alpha = true,
        .uses_subjacent_alpha = true,
        .default_pixel = 0x00000000,
        .alpha = 0xFF,
        .position = { L1X, L1Y },
        .pixels = {
            .pitch = sizeof *layer1_pixel_buf,
            .format = PF_L8,
            .w = L1W,
            .h = L1H,
            .pixels = layer1_pixel_buf,
        },
        .clut = (lcd_clut *)layer1_clut_buf,
    },
    .layer2 = {
        .is_enabled = true,
        .uses_pixel_alpha = true,
        .uses_subjacent_alpha = true,
        .default_pixel = 0x00000000,
        .alpha = 0xFF,
        .position = { 150, 150 },
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
    for (size_t y = 0; y < L1H; y++)
        for (size_t x = 0; x < L1W; x++)
            layer1_pixel_buf[y][x] = 0xFF00 | (x ^ y) & 0xFF;
}

static void draw_layer_2(void)
{
    size_t cx = L2W / 2;
    size_t cy = L2H / 2;
    for (size_t y = 0; y < L2H; y++) {
        for (size_t x = 0; x < L2W; x++) {
            uint8_t a = 0;
            if ((size_t)(abs(x - cx) + abs(y - cy)) < cx) {
                // // a = bit_reverse[(x + y) ^ (x - y) & 0xFF];
                // // a = 0xFF;
                // // uint16_t r = (x - cx) * (y - cy);
                // // a = (r & 0xFF) ^ (r >> 8);
                // uint8_t xd = abs(x - cx), yd = abs(y - cy);
                // uint8_t r = xd > yd ? xd : yd;
                // // uint8_t r = xd + yd;
                // a = 0x50 + 2 * abs(cx/2 - r);
                a = 64 | (x ^ y);
            }
            uint8_t c = ((x + y) ^ (x - y)) & 0xFF;
            // uint8_t c = (((x + y) ^ (x - y)) >> 0) & 8 ? 0xFF : 0x00;
            layer2_pixel_buf[y][x] = a << 8 | c;
        }
    }
}

static void init_layer1_clut(void)
{
    // for (size_t i = 0; i < 512; i++)
    //     layer1_clut_buf[i] = i % 256 << 16;
    for (size_t i = 0; i < 256; i++)
        layer1_clut_buf[i] = i & 0x20 ? (i & 0x60) << 16 | 0x000033 : 0;
#if 0
    for (size_t i = 256; i < 512; i++) {
        uint8_t c = bit_reverse[i % 256];
        layer1_clut_buf[i] = c << 16 | abs(64 - c) << 0;
    }
#elif 0
    for (size_t i = 256; i < 512; i++)
        layer1_clut_buf[i] = (i % 256) << 16;
#else
    for (size_t i = 256; i < 512; i++)
        layer1_clut_buf[i] = (i % 256) << 8 | (i % 256);
#endif
    for (size_t i = 512; i < 768; i++)
        layer1_clut_buf[i] = layer1_clut_buf[i - 512];
}

static void init_layer2_clut(void)
{
    for (size_t i = 0; i < 256; i++)
        layer2_clut_buf[i] = 0x0000FF | i << 9 | i << 16;
    for (size_t i = 256; i < 512; i++)
        layer2_clut_buf[i] = (0xFF - abs(384 - i)) << 8;
    for (size_t i = 512; i < 768; i++)
        layer2_clut_buf[i] = 0x0000FF | (i & 0xFF) << 9 | (i & 0xFF) << 16;
}

static lcd_settings *frame_callback(lcd_settings *s)
{
    static int l1x_inc = +1;
    static int l1y_inc = +1;
    static int l1c_index = 0;
    static int l2a_inc = +11;
    static int l2x_inc = +1;
    static int l2y_inc = +3;
    static int l2c_index = 0;

#if 0
    int x1 = s->layer1.position.x;
    x1 += l1x_inc;
    if (x1 > 0) {
        x1 = 0;
        l1x_inc *= -1;
    } else if (x1 < 800 - L1W) {
        x1 = 800 - L1W;
        l1x_inc *= -1;
    }
    s->layer1.position.x = x1;

    int y1 = s->layer1.position.y;
    y1 += l1y_inc;
    if (y1 > 0) {
        y1 = 0;
        l1y_inc *= -1;
    } else if (y1 < 480 - L1H) {
        y1 = 480 - L1W;
        l1y_inc *= -1;
    }
    s->layer1.position.y = y1;
#else
    static int l1s_inc = +2;

    int s1 = s->layer1.pixels.w;
    const size_t diag_pitch = s->layer1.pixels.pitch + sizeof (L1PF);
    intptr_t l1_addr = (intptr_t)s->layer1.pixels.pixels;
    l1_addr += (s1 - L1W) / 2 * diag_pitch;

    int cx1 = s->layer1.position.x + s1 / 2;
    cx1 += l1x_inc;
    if (cx1 < 800 - L1W / 2) {
        cx1 = 800 - L1W / 2;
        l1x_inc *= -1;
    } else if (cx1 > L1W / 2) {
        cx1 = L1W / 2;
        l1x_inc *= -1;
    }

    int cy1 = s->layer1.position.y + s1 / 2;
    cy1 += l1y_inc;
    if (cy1 < 480 - L1H / 2) {
        cy1 = 480 - L1H / 2;
        l1y_inc *= -1;
    } else if (cy1 > L1H / 2) {
        cy1 = L1H / 2;
        l1y_inc *= -1;
    }

    s1 += l1s_inc;
    if (s1 < 0) {
        s1 = 0;
        l1s_inc *= -1;
    } else if (s1 > L1W) {
        s1 = L1W;
        l1s_inc *= -1;
    }

    s->layer1.pixels.pixels = (void *)(l1_addr + (L1W - s1) / 2 * diag_pitch);
    s->layer1.position.x = cx1 - s1 / 2;
    s->layer1.position.y = cy1 - s1 / 2;
    s->layer1.pixels.w = s1;
    s->layer1.pixels.h = s1;
#endif

    int a2 = s->layer2.alpha;
    a2 += l2a_inc;
    if (a2 > 0xFF) {
        a2 = 0xFF - a2;
        l2a_inc *= -1;
    } else if (a2 < 0) {
        a2 = -a2;
        l2a_inc *= -1;
    }
    s->layer2.alpha = a2;

    int x2 = s->layer2.position.x;
    x2 += l2x_inc;
    if (x2 < 2 - L2W) {
        x2 = 2 - L2W;
        l2x_inc *= -1;
    } else if (x2 > 800 - 2) {
        x2 = 800 - 2;
        l2x_inc *= -1;
    }
    s->layer2.position.x = x2;

    int y2 = s->layer2.position.y;
    y2 += l2y_inc;
    if (y2 < 2 - L2H) {
        y2 = 2 - L2H;
        l2y_inc *= -1;
    } else if (y2 > 480 - 2) {
        y2 = 480 - 2;
        l2y_inc *= -1;
    }
    s->layer2.position.y = y2;

    s->layer1.clut = (lcd_clut *)(layer1_clut_buf + l1c_index);
    l1c_index++;
    l1c_index %= 512;

    s->layer2.clut = (lcd_clut *)(layer2_clut_buf + l2c_index);
    l2c_index += 3;
    l2c_index %= 512;

    return s;
}

int main(void)
{
    init_clocks(&MY_CLOCKS);
    systick_setup(MY_CLOCKS.ahb_frequency);
    lcd_pwm_setup();
    init_lcd(&MY_SCREEN);
    init_sdram();

    init_bitrev();
    draw_layer_1();
    draw_layer_2();
    init_layer1_clut();
    init_layer2_clut();
    lcd_set_frame_callback(frame_callback);
    lcd_load_settings(&my_settings, false);
    lcd_fade(0, 65535, 0, 2000);
    while (1)
        continue;
}
