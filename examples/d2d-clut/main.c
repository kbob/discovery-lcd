#define L1PF_RGB565
#define DOUBLE

#include "clock.h"
#include "dma2d.h"
#include "intr.h"
#include "lcd.h"
#include "lcd-pwm.h"
#include "sdram.h"
#include "systick.h"

#define MY_CLOCKS (rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ])
#define MY_SCREEN adafruit_p1596_lcd

#ifdef L1PF_RGB565
    #define L1PF rgb565
#else
    #define L1PF rgb888
#endif
#define L1H 480
#define L1W 800

#define IH 100
#define IW 100
#define ICL 50                  // image CLUT length

#define ROWS 3
#define COLS 5

#define DMA_REQ_COUNT 10
#define SRAM __attribute__((section(".system_ram")));


#ifdef DOUBLE
    static L1PF layer1_pixel_buf_A[L1H][L1W] SDRAM_BANK_0;
    static L1PF layer1_pixel_buf_B[L1H][L1W] SDRAM_BANK_1;
    static L1PF (*layer1_pixel_front)[L1W] = layer1_pixel_buf_A;
    static L1PF (*layer1_pixel_back)[L1W] = layer1_pixel_buf_B;
#else
    static L1PF layer1_pixel_buf[L1H][L1W] SDRAM_BANK_0;
#endif
static l8 image_l8[IH][IW] SRAM;
static argb8888 image_cluts[ROWS * COLS][ICL] SRAM;
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
#ifdef DOUBLE
            .pitch = sizeof *layer1_pixel_front,
#else
            .pitch = sizeof *layer1_pixel_buf,
#endif
#ifdef L1PF_RGB565
            .format = PF_RGB565,
#else
            .format = PF_RGB888,
#endif
            .w = L1W,
            .h = L1H,
#ifdef DOUBLE
            .pixels = layer1_pixel_buf_A,
#else
            .pixels = layer1_pixel_buf,
#endif
        },
    },
};

static void draw_layer_1(void)
{
    for (size_t y = 0; y < L1H; y++) {
        for (size_t x = 0; x < L1W; x++) {
#ifdef L1PF_RGB565
    #ifdef DOUBLE
            layer1_pixel_buf_A[y][x] = 0x7BCF;
            layer1_pixel_buf_B[y][x] = 0x7BCF;
    #else
            layer1_pixel_buf[y][x] = 0x7BCF;
    #endif
#else
    #ifdef DOUBLE
            layer1_pixel_buf_A[y][x] = (rgb888){ 0x7F, 0x7F, 0x7F };
            layer1_pixel_buf_B[y][x] = (rgb888){ 0x7F, 0x7F, 0x7F };
    #else
            layer1_pixel_buf[y][x] = (rgb888){ 0x7F, 0x7F, 0x7F };
    #endif
#endif
        }
    }
}

static void draw_image(void)
{
    for (int y = 0; y < 50; y++) {
        for (int x = 0; x < 50; x++) {
            uint8_t i = (x < y) ? x : y;
            image_l8[y][99 - x] =
                image_l8[y][x] = i % ICL;
        }
    }
    for (int y = 50; y < 100; y++)
        for (int x = 0; x < 100; x++)
            image_l8[y][x] = image_l8[99 - y][x];
}

#ifdef DOUBLE
static void swap_buffers(void)
{
    WITH_INTERRUPTS_MASKED {
        L1PF (*t)[L1W] = layer1_pixel_front;
        layer1_pixel_front = layer1_pixel_back;
        layer1_pixel_back = t;
    }
}
#endif

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
        0x60ff00,
    };
    static int rowcol;
    static int clut_rotor;

    int row = rowcol / 5;
    int col = rowcol % 5;
    size_t cix = rowcol;

    int dest_x = 88 + 132 * col;
    int dest_y = 60 + 132 * row;

    for (size_t i = 0; i < ICL; i++)
        image_cluts[rowcol][i] = 0xFF000000;
    uint32_t c = colors[cix];
    for (int i = 0, j = clut_rotor; i < 5; i++, j = (j + 4) % ICL) {
        image_cluts[rowcol][j] = 0xFF000000 | c;
        uint32_t r = c >> 16 & 0xFF;
        uint32_t g = c >>  8 & 0xFF;
        uint32_t b = c >>  0 & 0xFF;
        r = r * 5 / 6;
        g = g * 5 / 6;
        b = b * 5 / 6;
        c = r << 16 | g << 8 | b << 0;
    }

    dma2d_enqueue_clut_request(false, true, image_cluts[rowcol], ICL, NULL);

    pixmap dest = {
        .pitch = my_settings.layer1.pixels.pitch,
        .format = my_settings.layer1.pixels.format,
        .w = 100,
        .h = 100,
#ifdef DOUBLE
        .pixels = layer1_pixel_back,
#else
        .pixels = layer1_pixel_buf,
#endif
    };
    dest.pixels = pixmap_pixel_address(&dest, dest_x, dest_y);

    // indexed color source format
    pixmap src = {
        .pitch = sizeof image_l8[0],
        .format = PF_L8,
        .w = IW,
        .h = IH,
        .pixels = image_l8,
    };
    dma2d_enqueue_pfc_request(&dest, &src,
                              0,
                              DAM_SRC, 0,
                              NULL);
    rowcol = (rowcol + 1) % (3 * 5);
    if (rowcol == 0)
        clut_rotor = (clut_rotor + 1) % ICL;
}

static lcd_settings *frame_callback(lcd_settings *s)
{
    frame_count++;
#ifdef DOUBLE
    s->layer1.pixels.pixels = layer1_pixel_front;
    return s;
#else
    (void)s;
    return NULL;
#endif
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
    draw_image();
    dma2d_set_dead_time(128);
    lcd_set_frame_callback(frame_callback);
    lcd_load_settings(&my_settings, false);
    lcd_fade(0, 65535, 0, 2000);
    int fc = frame_count;
    while (1) {
        if (fc < frame_count) {
            fc = frame_count;
#ifdef DOUBLE
            swap_buffers();
#endif
            for (int i = 0; i < 15; i++)
                composite_image();
        }
    }
}
