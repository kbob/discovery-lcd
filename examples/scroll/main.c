#include "clock.h"
#include "intr.h"
#include "lcd.h"
#include "lcd-pwm.h"
#include "sdram.h"
#include "systick.h"

#define MY_CLOCKS (rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ])
#define MY_SCREEN adafruit_p1596_lcd

#define PF argb4444
#define H  480
#define W  800
#define T   60
#define T2 120

static PF pixel_buf[H][W] SDRAM_BANK_0;
static uint16_t x_steps[T], y_steps[T];

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
            .pitch = sizeof *pixel_buf,
            .format = PF_ARGB4444,
            .w = W,
            .h = H,
            .pixels = pixel_buf,
        },
    },
    .layer2 = {
        .is_enabled = true,
        .uses_pixel_alpha = true,
        .uses_subjacent_alpha = true,
        .default_pixel = 0x00000000,
        .alpha = 0xFF,
        .position = { 0, 0 },
        .pixels = {
            .pitch = sizeof *pixel_buf,
            .format = PF_ARGB4444,
            .w = W,
            .h = H,
            .pixels = pixel_buf,
        },
    },
};

volatile int frame_counter;
volatile int fps;

static void draw_pixels(void)
{
    const size_t cx = W/2, cy = H/2;
    for (size_t y = 0; y < H; y++)
        for (size_t x = 0; x < W; x++) {
            int r2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
            uint8_t c = r2 * (8 * 255) / (cx * cx + cy * cy);
            uint8_t c4 = c >> 4;
            pixel_buf[y][x] = ((H + x - y) >> 3) & 1
                ? 0xF440 | c4
                : 0xF044 | c4 << 8;
        }
}

static void init_smoothsteps(void)
{
    for (int i = 0; i < T; i++) {
        float s = (float)i / (float)(T-1);
        s = s * s * (3 - 2 * s);
        x_steps[i] = s * W;
        y_steps[i] = s * H;
    }
}

static lcd_settings *frame_callback(lcd_settings *s)
{
    frame_counter++;

    int xo = 0, yo = 0;

    static int frame_num;
    int i = frame_num % T2;
    int j = frame_num / T2 % 4;
    frame_num++;
    if (i < T) {
        switch (j) {

        case 0:                 // scroll right
            xo = x_steps[i];
            break;
                
        case 1:                 // scroll left
            xo = W - x_steps[i];
            break;
                
        case 2:                 // scroll down
            yo = y_steps[i];
            break;
                
        case 3:                 // scroll up
            yo = H - y_steps[i];
            break;
        }
    }
    if (xo || yo) {
        s->layer1.is_enabled = true;
        s->layer1.pixels.pixels =
            pixmap_pixel_address(&s->layer2.pixels,
                                 xo ? W - xo : 0,
                                 yo ? H - yo : 0);
        s->layer1.pixels.w = xo ? xo : W;
        s->layer1.pixels.h = yo ? yo : H;
    } else {
        s->layer1.is_enabled = false;
    }
    s->layer2.position.x = xo;
    s->layer2.position.y = yo;
    s->layer2.pixels.w = W - xo;
    s->layer2.pixels.h = H - yo;
        
    return s;
}

static void update_fps(void)
{
    static uint32_t next_time;
    uint32_t now = system_millis;
    
    if (now >= next_time) {
        WITH_INTERRUPTS_MASKED {
            fps = frame_counter;
            frame_counter = 0;
        }
        next_time += 1000;
    }
}

int main(void)
{
    init_clocks(&MY_CLOCKS);
    systick_setup(MY_CLOCKS.ahb_frequency);
    lcd_pwm_setup();
    init_lcd(&MY_SCREEN);
    init_sdram();

    draw_pixels();
    init_smoothsteps();
    lcd_set_frame_callback(frame_callback);
    lcd_load_settings(&my_settings, false);
    lcd_fade(0, 65535, 0, 2000);
    while (1)
        update_fps();
}
