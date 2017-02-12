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

static void draw_solid(void)
{
    static int rowcol;
    static int color;
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
    dma2d_enqueue_solid_request(&dest, 0xFF000000 | colors[color], NULL);

    rowcol = (rowcol + 1) % (3 * 5);
    color = (color + 1) % ((&colors)[1] - colors);
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
            draw_solid();
        }
    }
}
