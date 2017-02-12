#include "clock.h"
#include "dma2d.h"
#include "lcd.h"
#include "lcd-pwm.h"
#include "sdram.h"
#include "systick.h"

#define MY_CLOCKS (rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ])
#define MY_SCREEN adafruit_p1596_lcd

#define L1PF rgb888
#define L1H 480
#define L1W 800

#define DMA_REQ_COUNT 10

static L1PF layer1_pixel_buf[L1H][L1W] SDRAM_BANK_0;
static dma2d_request dma_requests[DMA_REQ_COUNT];
static volatile int frame_count;

static lcd_settings my_settings = {
    .bg_pixel = 0xFF0000,
    .layer1 = {
        .is_enabled = true,
        .uses_pixel_alpha = true,
        .uses_subjacent_alpha = true,
        .default_pixel = 0x00000000,
        .alpha = 0xFF,
        .position = { 0, 0 },
        .pixels = {
            .pitch = sizeof *layer1_pixel_buf,
            .format = PF_RGB888,
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
            layer1_pixel_buf[y][x] = (rgb888) { 0x7f, 0x7f, 0x7f };
}

static void draw_image(void)
{
    static const L1PF colors[] = {
        { 0xff, 0x4a, 0x00 },
        { 0x00, 0xff, 0x95 },
        { 0xdf, 0x00, 0xff },
        { 0xd4, 0xff, 0x00 },
        { 0x00, 0x8a, 0xff },
        { 0xff, 0x00, 0x40 },
        { 0x00, 0xff, 0x0b },
        { 0x55, 0x00, 0xff },
        { 0xff, 0xa0, 0x00 },
        { 0x00, 0xff, 0xea },
        { 0xff, 0x00, 0xca },
        { 0x7f, 0xff, 0x00 },
        { 0x00, 0x35, 0xff },
        { 0xff, 0x16, 0x00 },
        // { 0x00, 0xff, 0x60 },
    };
    static const size_t nc = (&colors)[1] - colors;
    static L1PF images[2][100][100] __attribute__((section(".system_ram")));
    static L1PF (*image)[100] = images[0];
    static int rowcol;
    static int cix;

    int row = rowcol / 5;
    int col = rowcol % 5;

    pixmap dest = {
        .pitch = my_settings.layer1.pixels.pitch,
        .format = my_settings.layer1.pixels.format,
        .w = 100,
        .h = 100,
        .pixels = pixmap_pixel_address(&my_settings.layer1.pixels,
                                       88 + 132 * col,
                                       60 + 132 * row),
    };
    pixmap src = {
        .pitch = sizeof image[0],
        .format = my_settings.layer1.pixels.format,
        .w = 100,
        .h = 100,
        .pixels = image,
    };
    dma2d_enqueue_copy_request(&dest, &src, NULL);
    image = image == images[0] ? images[1] : images[0];

    L1PF c0 = colors[(cix + 0) % nc];
    L1PF c1 = colors[(cix + 1) % nc];
    L1PF c2 = colors[(cix + 2) % nc];
    L1PF c3 = colors[(cix + 3) % nc];

    for (int y = 0; y < 50; y++) {
        int x = 0;
        for ( ; x < y; x++)
            image[y][x] = c0;
        for ( ; x < 100 - y; x++)
            image[y][x] = c1;
        for ( ; x < 100; x++)
            image[y][x] = c3;
    }
    for (int y = 50; y < 100; y++) {
        int x = 0;
        for ( ; x < 100 - y; x++)
            image[y][x] = c0;
        for ( ; x < y; x++)
            image[y][x] = c2;
        for ( ; x < 100; x++)
            image[y][x] = c3;
    }

    rowcol = (rowcol + 1) % (3 * 5);
    cix = (cix + 4) % nc;
}

static lcd_settings *frame_callback(lcd_settings *s)
{
    (void)s;
    frame_count++;
    return NULL;
}

int main(void)
{
    init_clocks(&MY_CLOCKS);
    systick_setup(MY_CLOCKS.ahb_frequency);
    lcd_pwm_setup();
    init_lcd(&MY_SCREEN);
    init_sdram();
    init_dma2d(dma_requests, DMA_REQ_COUNT);

    draw_layer_1();
    lcd_set_frame_callback(frame_callback);
    lcd_load_settings(&my_settings, false);
    lcd_fade(0, 65535, 0, 2000);
    int fc = frame_count;
    while (1) {
        if (fc + 2 < frame_count) {
            fc = frame_count;
            draw_image();
        }
    }
}
