#include <stdlib.h>

#include <libopencm3/cm3/cortex.h>

#include "clock.h"
#include "dma2d.h"
#include "lcd.h"
#include "lcd-pwm.h"
#include "sdram.h"
#include "systick.h"

#include "fhpn-pixmap.inc"

#define MY_CLOCKS (rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ])
#define MY_SCREEN adafruit_p1596_lcd

#define L1PF  rgb565
#define L1PFE PF_RGB565
#define L1H   480
#define L1W   800

#define DMA_REQ_COUNT 16

#define BG_COLOR_888 0x11083a
#define FG_COLOR_888 0xffb229
#define BG_COLOR     0x1047

static L1PF layer1_pixel_buf_A[L1H][L1W] SDRAM_BANK_0;
static L1PF layer1_pixel_buf_B[L1H][L1W] SDRAM_BANK_1;

static pixmap layer1_pixmap_A = {
    .pitch  = sizeof *layer1_pixel_buf_A,
    .format = L1PFE,
    .w      = L1W,
    .h      = L1H,
    .pixels = layer1_pixel_buf_A,
};
static pixmap layer1_pixmap_B = {
    .pitch  = sizeof *layer1_pixel_buf_B,
    .format = L1PFE,
    .w      = L1W,
    .h      = L1H,
    .pixels = layer1_pixel_buf_B,
};

static pixmap *layer1_front = &layer1_pixmap_A;
static pixmap *layer1_back = &layer1_pixmap_B;

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
            .pitch = sizeof *layer1_pixel_buf_A,
            .format = L1PFE,
            .w = L1W,
            .h = L1H,
            .pixels = layer1_pixel_buf_A,
        },
    },
};

static void draw_layer_1(void)
{
    for (size_t y = 0; y < L1H; y++) {
        for (size_t x = 0; x < L1W; x++) {
            layer1_pixel_buf_A[y][x] = BG_COLOR;
            layer1_pixel_buf_B[y][x] = BG_COLOR;
            // if (x % 32 == 0) {
            //     layer1_pixel_buf_A[y][x] = 0xFFFF;
            //     layer1_pixel_buf_B[y][x] = 0xFFFF;
            // }
            // if (y == 10 || y == 50) {
            //     layer1_pixel_buf_A[y][x] = 0x03C0;
            //     layer1_pixel_buf_B[y][x] = 0x03C0;
            // }
        }
    }
}

static void swap_buffers(void)
{
    CM_ATOMIC_BLOCK() {
        pixmap *t = layer1_front;
        layer1_front = layer1_back;
        layer1_back = t;
    }
}

static void composite_image(void)
{
    static int x_inc = +1;
    static int x = 100;
    static int prev_x, prev_prev_x;
    prev_prev_x = prev_x;
    prev_x = x;
    x += x_inc;
    if (x > 200) {
        x = 200;
        x_inc = -x_inc;
    }
    if (x < 100) {
        x = 100;
        x_inc = -x_inc;
    }

    int base_y = 50;
    for (int i = 0; i < 4; i++) {
        int ascender_y = base_y - fhpn.ascent;

        // if (i % 2) {
        //     pixmap pm = {
        //         .pitch  = layer1_back->pitch,
        //         .format = layer1_back->format,
        //         .w      = fhpn.pixels.w + 100,
        //         .h      = fhpn.line_height,
        //         .pixels = pixmap_pixel_address(layer1_back,
        //                                        100,
        //                                        base_y - fhpn.line_height),
        //     };
        //     dma2d_enqueue_solid_request(&pm, 0xFFFF, NULL);
        // }

        pixmap dest = {
            .pitch  = layer1_back->pitch,
            .format = layer1_back->format,
            .w      = fhpn.pixels.w,
            .h      = fhpn.pixels.h,
            .pixels = pixmap_pixel_address(layer1_back, x, ascender_y),
        };

        // Use System RAM as source for background pixels.  Pixels
        // are read, but their values do not affect the final value.
        extern uint8_t system_ram_start[];
        void *bg_pixels = system_ram_start;
        pixmap bg_pixmap = {
            .pitch  = 0,
            .format = PF_A4,
            .w      = fhpn.pixels.w,
            .h      = fhpn.pixels.h,
            .pixels = bg_pixels,
        };
        dma2d_enqueue_blend_request(&dest,
                                    &fhpn.pixels,
                                    &bg_pixmap,
                                    FG_COLOR_888,
                                    BG_COLOR_888,
                                    DAM_PRODUCT,
                                    DAM_REQ,
                                    0xFF,
                                    0xFF,
                                    NULL);
        if (prev_prev_x && prev_prev_x < x) {
            dest.w = x - prev_prev_x;
            dest.pixels = pixmap_pixel_address(layer1_back,
                                               prev_prev_x, ascender_y);
            dma2d_enqueue_solid_request(&dest, BG_COLOR, NULL);
        } else if (prev_prev_x > x) {
            dest.w = prev_prev_x - x;
            dest.pixels = pixmap_pixel_address(layer1_back,
                                               x + fhpn.pixels.w, ascender_y);
            dma2d_enqueue_solid_request(&dest, BG_COLOR, NULL);
        }
        base_y += fhpn.line_height;
    }
}

static lcd_settings *frame_callback(lcd_settings *s)
{
    frame_count++;
    s->layer1.pixels.pixels = layer1_front->pixels;
    return s;
}

int main(void)
{
    init_clocks(&MY_CLOCKS);
    systick_setup(MY_CLOCKS.ahb_frequency);
    lcd_pwm_setup();
    init_lcd(&MY_SCREEN);
    init_sdram();
    // dma2d_set_dead_time(128);
    init_dma2d(dma_requests, DMA_REQ_COUNT);

    draw_layer_1();
    lcd_set_frame_callback(frame_callback);
    lcd_load_settings(&my_settings, false);
    lcd_fade(0, 65535, 0, 2000);
    int fc = frame_count;
    while (1) {
        if (fc + 2 < frame_count) {
            fc = frame_count;
            swap_buffers();
            composite_image();
        }
    }
}
