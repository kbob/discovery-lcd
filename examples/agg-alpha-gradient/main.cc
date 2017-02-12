// C++ std headers
#include <algorithm>

// C/POSIX headers
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// External Library Headers
#include <libopencm3/cm3/dwt.h>
#include <libopencm3/stm32/gpio.h>

#include "platform/agg_platform_support.h"

// Project Headers
#include "clock.h"
#include "intr.h"
#include "lcd.h"
#include "lcd-pwm.h"
#include "sdram.h"
#include "systick.h"

#define MY_CLOCKS (rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ])
#define MY_SCREEN adafruit_p1596_lcd

#define L1PF rgb888
#define L1H 480
#define L1W 800

#define L2PF al44
#define L2H 256
#define L2W 256

static L1PF     layer1_pixel_buf_0[L1H][L1W] SDRAM_BANK_0;
static L1PF     layer1_pixel_buf_1[L1H][L1W] SDRAM_BANK_1;
static L1PF   (*layer1_front_buffer)[L1W] = layer1_pixel_buf_0;
static L1PF   (*layer1_back_buffer)[L1W] = layer1_pixel_buf_1;
static bool     lcd_started;
static uint32_t timer_zero;

// static L2PF  layer2_buffer[L2H][L2W] SDRAM_BANK_2;

static lcd_settings my_settings;

static void init_lcd_settings(lcd_settings *s)
{
    s->bg_pixel = 0x7f7f7f;
    s->layer1.is_enabled = true;
    s->layer1.uses_pixel_alpha = false;
    s->layer1.uses_subjacent_alpha = false;
    s->layer1.alpha = 0xFF;
    s->layer1.position.x = 0;
    s->layer1.position.y = 0;
    s->layer1.pixels.pitch = sizeof *layer1_pixel_buf_0;
    s->layer1.pixels.format = PF_RGB888;
    s->layer1.pixels.w = L1H;
    s->layer1.pixels.h = L1W;
    s->layer1.pixels.pixels = layer1_back_buffer;
}

static lcd_settings *frame_callback(lcd_settings *s)
{
    s->layer1.pixels.pixels = layer1_back_buffer;
    return s;
}

static void flip_layer1()
{
    WITH_INTERRUPTS_MASKED {
        L1PF (*t)[L1W] = layer1_front_buffer;
        layer1_front_buffer = layer1_back_buffer;
        layer1_back_buffer = t;
    }
    if (!lcd_started) {
        lcd_set_frame_callback(frame_callback);
        lcd_load_settings(&my_settings, false);
        lcd_fade(0, 65535, 0, 2000);
        lcd_started = true;
    }
}

namespace agg {

    class platform_specific {
    public:
        platform_specific()
        {}
    private:
    };

    platform_support::platform_support(pix_format_e format, bool flip_y)
        : m_format(format),
          m_bpp(24),
          m_rbuf_window(),
          m_rbuf_img(),
          m_window_flags(0),
          m_wait_mode(true),
          m_flip_y(flip_y),
          m_caption(""),
          m_initial_width(L1W),
          m_initial_height(L1H),
          m_resize_mtx()
    {}

    platform_support::~platform_support() {}

    void platform_support::caption(const char *) {}

    bool platform_support::init(unsigned width, unsigned height, unsigned flags)
    {
        unsigned w = std::min((unsigned)L1W, width);
        unsigned h = std::min((unsigned)L1H, height);
        my_settings.layer1.pixels.w = w;
        my_settings.layer1.pixels.h = h;
        my_settings.layer1.position.x = (L1W - w) / 2;
        my_settings.layer1.position.y = (L1H - h) / 2;
        m_rbuf_window.attach((agg::int8u *)layer1_front_buffer,
                             w, h,
                             m_flip_y ?
                                 -my_settings.layer1.pixels.pitch :
                                 +my_settings.layer1.pixels.pitch);
        m_initial_width = width;
        m_initial_height = height;
        on_init();
        return true;
    }

    int platform_support::run()
    {
        force_redraw();
        while (true) {
            if (!m_wait_mode)
                force_redraw();
        }
        return EXIT_SUCCESS;
    }

    void platform_support::force_redraw()
    {
        on_draw();
        flip_layer1();
    }

    void platform_support::update_window()
    {
        flip_layer1();
    }

    void platform_support::on_init() {}
    void platform_support::on_resize(int sx, int sy) {}
    void platform_support::on_idle() {}
    void platform_support::on_mouse_move(int x, int y, unsigned flags) {}
    void platform_support::on_mouse_button_down(int x, int y, unsigned flags) {}
    void platform_support::on_mouse_button_up(int x, int y, unsigned flags) {}
    void platform_support::on_key(int x, int y, unsigned key, unsigned flags) {}
    void platform_support::on_ctrl_change() {}
    void platform_support::on_draw() {}
    void platform_support::on_post_draw(void* raw_handler) {}

    void platform_support::message(const char *msg) {}

    void platform_support::start_timer()
    {
        bool ok = dwt_enable_cycle_counter();
        assert(ok);
        timer_zero = dwt_read_cycle_counter();
    }

    double platform_support::elapsed_time() const
    {
        uint32_t t1 = dwt_read_cycle_counter();
        return (double)(t1 - timer_zero) * 1000.0 / MY_CLOCKS.ahb_frequency;
    }

    

}

int main()
{
    init_clocks(&MY_CLOCKS);
    systick_setup(MY_CLOCKS.ahb_frequency);
    lcd_pwm_setup();
    init_lcd(&MY_SCREEN);
    init_sdram();

    init_lcd_settings(&my_settings);

    extern int agg_main(int argc, char* argv[]);
    char *argv[] = { strdup("program"), NULL };
    agg_main(1, argv);
}

extern "C" void abort(void)
{
    cm_disable_interrupts();
    rcc_periph_clock_enable(RCC_GPIOG);
    gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO14);
    while (true) {
        gpio_set(GPIOG, GPIO14);
        for (int i = 0; i < 1000000; i++)
            __asm volatile ("nop");
        gpio_clear(GPIOG, GPIO14);
        for (int i = 0; i < 1000000; i++)
            __asm volatile ("nop");
    }
}

extern "C" void *_sbrk(int incr)
{
    extern void *system_ram_start, *system_ram_end;
    static uintptr_t ptr = (uintptr_t)&system_ram_start;
    static const uintptr_t top_of_RAM = (uintptr_t)&system_ram_end;
    if (ptr + incr > top_of_RAM) {
        errno = ENOMEM;
        return (void *)-1;
    }
    void *ret = (void *)ptr;
    ptr += incr;
    return ret;
}

extern "C" void __assert_func(const char *, int, const char *, const char *)
{
    abort();
}

extern "C" void _exit(int status)
{
    abort();
}

extern "C" void _open()
{
    abort();
}

extern "C" void _fstat()
{
    abort();
}

extern "C" void _write()
{
    abort();
}

extern "C" void _close()
{
    abort();
}

extern "C" void _isatty()
{
    abort();
}

extern "C" void _read()
{
    abort();
}

extern "C" void _lseek()
{
    abort();
}
