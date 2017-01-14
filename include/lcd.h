#ifndef LCD_included
#define LCD_included

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "pixfmts.h"

// unchanging configuration:
//  screen size
//  refresh rate
//  timings: vert/horiz blank, front/back porch



typedef struct lcd_config {
    uint16_t width;
    uint16_t height;
    union {
        struct {
            uint32_t sain;
            uint32_t sair;      // if sair nonzero, it's sain/sair.
        };
        uint32_t dotclock;      // if > 10000, it's dotclock (Hz).
        uint32_t refresh;       // if <= 10000, it's refresh (Hz).
    };
    uint16_t h_sync;
    uint16_t h_back_porch;
    uint16_t h_front_porch;
    uint16_t v_sync;
    uint16_t v_back_porch;
    uint16_t v_front_porch;
    bool     use_dither;
    // XXX might need to set polarity for vsync, hsync. DE, dotclock.
} lcd_config;

typedef struct ipoint {
    int       x, y;
} ipoint;

typedef struct pixmap {
    ssize_t   pitch;            // row pitch in bytes
    pixfmt    format;
    size_t    w, h;
    void     *pixels;
} pixmap;

typedef xrgb_888 clut16[16];
typedef xrgb_888 clut256[256];

typedef union lcd_clut {
    clut16    c16;
    clut256   c256;
} lcd_clut;

typedef struct lcd_layer_settings {
    bool      is_enabled;
    bool      uses_pixel_alpha;
    bool      uses_subjacent_alpha;
    bool      uses_color_key;
    argb8888  default_pixel;
    xrgb_888  color_key_pixel;
    uint8_t   alpha;
    ipoint    position;
    pixmap    pixels;
    lcd_clut *clut;             // NULL for no CLUT
} lcd_layer_settings;

typedef struct lcd_settings {
    argb8888  bg_pixel;
    uint16_t  interrupt_line;
    lcd_layer_settings layer1;
    lcd_layer_settings layer2;
} lcd_settings;

typedef lcd_settings *lcd_settings_callback(lcd_settings *);

extern const lcd_config discovery_lcd;

// https://www.adafruit.com/product/1596
extern const lcd_config adafruit_p1596_lcd;

extern void init_lcd(const lcd_config *);
extern void lcd_set_frame_callback(lcd_settings_callback *);
extern void lcd_set_line_callback(lcd_settings_callback *);

extern void lcd_load_settings(const lcd_settings *, bool immediate);

#endif /* !LCD_included */
