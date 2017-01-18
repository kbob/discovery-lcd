#include "clock.h"
#include "intr.h"
#include "lcd.h"
#include "lcd-pwm.h"
#include "sdram.h"
#include "systick.h"

#define MY_CLOCKS (rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ])
#define MY_SCREEN adafruit_p1596_lcd

#define L1PF argb4444
#define L1H 100
#define L1W 400

#define L2PF al44
#define L2H 256
#define L2W 256

static L1PF layer1_buffer[L1H][L1W] SDRAM_BANK_0;
static L2PF layer2_buffer[L2H][L2W] SDRAM_BANK_2;

static lcd_settings my_settings = {
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

volatile int frame_counter;
volatile int fps;

static void draw_layer_1(void)
{
    for (size_t y = 0; y < L1H; y++)
        for (size_t x = 0; x < L1W; x++)
            layer1_buffer[y][x] = ((L1H + x - y) >> 3) & 1 ? 0xF00F : 0x50F0;
            // layer1_buffer[y][x] = ((L1H + x - y) >> 3) & 1 ? 0xFF0000FF : 0x5500FF00;
}

static void draw_layer_2(void)
{
    for (size_t y = 0; y < L2H; y++)
        for (size_t x = 0; x < L2W; x++)
            layer2_buffer[y][x] = y >> 4 << 4 | x >> 4;
}

static lcd_settings *frame_callback(lcd_settings *s)
{
    frame_counter++;

    static int l1x_inc = +2;
    static int l2y_inc = +2;

    s->layer1.position.x += l1x_inc;
    if (s->layer1.position.x > 800 - 2) {
        s->layer1.position.x = 800 - 2;
        l1x_inc *= -1;
    } else if (s->layer1.position.x < 2 - L1W) {
        s->layer1.position.x = 2 - L1W;
        l1x_inc *= -1;
    }

    s->layer2.position.y += l2y_inc;
    if (s->layer2.position.y > 480 - 2) {
        s->layer2.position.y = 480 - 2;
        l2y_inc *= -1;
    } else if (s->layer2.position.y < 2 - L2H) {
        s->layer2.position.y = 2 - L2W;
        l2y_inc *= -1;
    }

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

    draw_layer_1();
    draw_layer_2();
    lcd_set_frame_callback(frame_callback);
    lcd_load_settings(&my_settings, false);
    lcd_fade(0, 65535, 0, 2000);
    while (1)
        update_fps();
}
