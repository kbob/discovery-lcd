#include "clock.h"
#include "dma2d.h"
#include "lcd.h"
#include "lcd-pwm.h"
#include "sdram.h"
#include "systick.h"

#define MY_CLOCKS (rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ])
#define MY_SCREEN adafruit_p1596_lcd

#define L1PF argb4444
#define L1H 480
#define L1W 800

#define IH 100
#define IW 100

#define DMA_REQ_COUNT 10

static L1PF layer1_pixel_buf[L1H][L1W] SDRAM_BANK_0;
static a8 image_a8[IH][IW] __attribute__((section(".system_ram")));
static rgb565 image_rgb565[IH][IW] __attribute__((section(".system_ram")));
static dma2d_request dma_requests[DMA_REQ_COUNT];
static volatile int frame_count;

static lcd_settings my_settings = {
    .bg_pixel = 0xFFFF0000,
    .layer1 = {
        .is_enabled = true,
        .uses_pixel_alpha = true,
        .uses_subjacent_alpha = true,
        .default_pixel = 0x00000000,
        .alpha = 0xFF,
        .position = { 0, 0 },
        .pixels = {
            .pitch = sizeof *layer1_pixel_buf,
            .format = PF_ARGB4444,
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
            layer1_pixel_buf[y][x] = 0xF777;
}

static void draw_images(void)
{
    static rgb565 c0 = 0x025f;
    static rgb565 c1 = 0x97e0;
    static rgb565 c2 = 0xf81b;
    static rgb565 c3 = 0x07fa;

    // concentric squares
    for (int y = 0; y < 50; y++) {
        for (int x = 0; x < 50; x++) {
            uint8_t i = (x < y) ? x : y;
            image_a8[y][99 - x] =
                image_a8[y][x] = (i << 2) & 0xE0;
        }
    }
    for (int y = 50; y < 100; y++)
        for (int x = 0; x < 100; x++)
            image_a8[y][x] = image_a8[99 - y][x];


    // diagonal triangles
    for (int y = 0; y < 50; y++) {
        int x = 0;
        for ( ; x < y; x++) {
            image_rgb565[y][x] = c0;
        }
        for ( ; x < 100 - y; x++) {
            image_rgb565[y][x] = c1;
        }
        for ( ; x < 100; x++) {
            image_rgb565[y][x] = c3;
        }
    }
    for (int y = 50; y < 100; y++) {
        int x = 0;
        for ( ; x < 100 - y; x++) {
            image_rgb565[y][x] = c0;
        }
        for ( ; x < y; x++) {
            image_rgb565[y][x] = c2;
        }
        for ( ; x < 100; x++) {
            image_rgb565[y][x] = c3;
        }
    }
}

static void composite_image(void)
{
    static const uint32_t colors[] = {
        0x004aff,
        0x95ff00,
        0xff00df,
        0x00ffd4,
        0xff8a00,
        0x4000ff,
        0x0bff00,
        0xff0055,
        0x00a0ff,
        0xeaff00,
        0xca00ff,
        0x00ff7f,
        0xff3500,
        0x0016ff,
        // 0x60ff00,
    };
    static const size_t nc = (&colors)[1] - colors;
    static int rowcol;
    static int cix;
    static uint8_t alpha;

    int row = rowcol / 5;
    int col = rowcol % 5;

    int dest_x = 88 + 132 * col;
    int dest_y = 60 + 132 * row;

    pixmap dest = {
        .pitch = my_settings.layer1.pixels.pitch,
        .format = my_settings.layer1.pixels.format,
        .w = 100,
        .h = 100,
        .pixels = pixmap_pixel_address(&my_settings.layer1.pixels,
                                       dest_x, dest_y),
    };

    pixmap fg = {
        .pitch = sizeof image_a8[0],
        .format = PF_A8,
        .w = IW,
        .h = IH,
        .pixels = image_a8,
    };

    pixmap bg = {
        .pitch = sizeof image_rgb565[0],
        .format = PF_RGB565,
        .w = IW,
        .h = IH,
        .pixels = image_rgb565,
    };

    dma2d_enqueue_blend_request(&dest, &fg, &bg,
                                colors[cix], 0,
                                DAM_SRC, DAM_REQ,
                                0xFF, alpha,
                                NULL);

    rowcol = (rowcol + 1) % (3 * 5);
    cix = (cix + 4) % nc;
    alpha++;
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
    draw_images();
    lcd_set_frame_callback(frame_callback);
    lcd_load_settings(&my_settings, false);
    lcd_fade(0, 65535, 0, 2000);
    int fc = frame_count;
    while (1) {
        if (fc + 2 < frame_count) {
            fc = frame_count;
            composite_image();
        }
    }
}
