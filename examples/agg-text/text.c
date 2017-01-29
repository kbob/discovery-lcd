#define ENGLISH
//#define DEUTSCH
//#define POLSKA
#include "text.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H
#include FT_SYSTEM_H

#define FONT_SIZE (10 << 6)

static const uint32_t amber = 0xffffb229;

#ifdef DEUTSCH
static const uint32_t message[] = {
    0x201c, 'F', 'i', 'x', ',', ' ', 'S', 'c', 'h', 'w', 'y', 'z', '!', 0x201d,
    ' ', 'q', 'u', 0xe4, 'k', 't', ' ', 'J', 0xfc, 'r', 'g', 'e', 'n', ' ',
    'b', 'l', 0xf6, 'd', ' ', 'v', 'o', 'm', ' ', 'P', 'a', 0xdf, '.',
//    ' ', ' ', 0x266d, ' ', 0x266e, ' ', 0x266f, ' ', '1', '0', 0xa2,
    '\0'
};
#elif defined(POLSKA)
static const uint32_t message[] = {
    'P', 'c', 'h', 'n', 0x105, 0x107, ' ', 'w', ' ', 't', 0x119, ' ', 0x142,
    0xf3, 'd', 0x17a, ' ', 'j', 'e', 0x17c, 'a', ' ', 'l', 'u', 'b', ' ', 'o',
    0x15b, 'm', ' ', 's', 'k', 'r', 'z', 'y', 0x144, ' ', 'f', 'i', 'g', '.',
    '\0'
};

#elif defined(ENGLISH)
static const char message[] = "Squdgy fez, blank jimp crwth vox! illliill 11111111";
#endif

typedef void pixel_render_func(size_t x, size_t y,
                               xrgb_888 front, uint8_t alpha);

static FT_Library    library;
static FT_Face       face;
static bool          ft_is_initialized;
static const pixmap *output_pixmap;
static int           line_x26_6, line_y26_6;
static int           pen_x26_6, pen_y26_6;
static FT_UInt       prev_glyph_index;

#define CHECK(error) (assert(error == 0))

extern uint8_t _binary_examples_agg_text_Lato_Light_ttf_start[];
extern uint8_t _binary_examples_agg_text_Lato_Light_ttf_size[];

void init_text(void)
{
    FT_Error error = FT_Init_FreeType(&library);
    CHECK(error);
    error = FT_New_Memory_Face(library,
                               _binary_examples_agg_text_Lato_Light_ttf_start,
                               (FT_Long)_binary_examples_agg_text_Lato_Light_ttf_size,
                               0,
                               &face);

    CHECK(error);
    error = FT_Set_Char_Size(face,
                             0,
                             FONT_SIZE,
                             188,
                             188);
    CHECK(error);
    ft_is_initialized = true;
}

void text_set_cursor(pixmap *p, int x, int y)
{
    output_pixmap = p;
    pen_x26_6 = line_x26_6 = x << 6;
    pen_y26_6 = line_y26_6 = y << 6;
}

static rgb565 blend_pixel_rgb565(xrgb_888 front, rgb565 back, uint8_t alpha)
{
    if (alpha == 0)
        return back;
    uint32_t fr = front >> 16 & 0xFF, fg = front >> 8 & 0xFF, fb = front & 0xFF;
    if (alpha == 0xFF)
        return fr >> 3 << 11 | fg >> 2 << 5 | fb >> 3 << 0;
    uint32_t br5 = back >> 11 & 0x1F;
    uint32_t bg6 = back >>  5 & 0x3F;
    uint32_t bb5 = back >>  0 & 0x1F;
    uint32_t br8 = br5 << 3 | br5 >> 2;
    uint32_t bg8 = bg6 << 2 | bg6 >> 4;
    uint32_t bb8 = bb5 << 3 | bb5 >> 2;
    uint32_t a = alpha, na = 0xFF - alpha;
    uint32_t r = (a * fr + na * br8) / 0xFF;
    uint32_t g = (a * fg + na * bg8) / 0xFF;
    uint32_t b = (a * fb + na * bb8) / 0xFF;
    return r >> 3 << 11 | g >> 2 << 5 | b >> 3 << 0;
}

static rgb888 blend_pixel_rgb888(xrgb_888 front, rgb888 back, uint8_t alpha)
{
    if (alpha == 0)
        return back;
    uint32_t fr = front >> 16 & 0xFF, fg = front >> 8 & 0xFF, fb = front & 0xFF;
    if (alpha == 0xFF)
        return (rgb888) { .r = fr, .g = fg, .b = fb };
    uint32_t a = alpha, na = 0xFF - alpha;
    uint32_t r = (a * fr + na * back.r) / 0xFF;
    uint32_t g = (a * fg + na * back.g) / 0xFF;
    uint32_t b = (a * fb + na * back.b) / 0xFF;
    return (rgb888) { .r = r, .g = g, .b = b };
}

static void render_pixel_rgb565(size_t x, size_t y,
                                xrgb_888 front, uint8_t alpha)
{
    rgb565 *pix = pixmap_pixel_address(output_pixmap, x, y);
    *pix = blend_pixel_rgb565(front, *pix, alpha);
}

static void render_pixel_rgb888(size_t x, size_t y,
                                xrgb_888 front, uint8_t alpha)
{
    rgb888 *pix = pixmap_pixel_address(output_pixmap, x, y);
    *pix = blend_pixel_rgb888(front, *pix, alpha);
}

static void render_codepoint(uint32_t codepoint)
{
    assert(ft_is_initialized);

    FT_GlyphSlot slot = face->glyph;
    if (codepoint  == '\n') {
        pen_x26_6 = line_x26_6;
        pen_y26_6 = line_y26_6 += slot->metrics.vertAdvance;
        prev_glyph_index = 0;
        return;
    }

    pixel_render_func *render_pixel;
    switch (output_pixmap->format) {

    case PF_RGB888:
        render_pixel = render_pixel_rgb888;
        break;

    case PF_RGB565:
        render_pixel = render_pixel_rgb565;
        break;

    default:
        assert(false);
    }

    FT_UInt glyph_index = FT_Get_Char_Index(face, codepoint);
    assert(glyph_index);
    if (glyph_index == 0)
        return;

    if (prev_glyph_index) {
        FT_Vector kerning = { 0, 0 };
        FT_Error error = FT_Get_Kerning(face,
                                        prev_glyph_index,
                                        glyph_index,
                                        FT_KERNING_DEFAULT,
                                        &kerning);
        CHECK(error);
        pen_x26_6 += kerning.x;
        pen_y26_6 += kerning.y;
    }

    FT_Error error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    CHECK(error);
    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    CHECK(error);
    for (size_t i = 0; i < slot->bitmap.rows; i++) {
        for (size_t j = 0; j < slot->bitmap.width; j++) {
            size_t ii = (pen_y26_6 >> 6) - slot->bitmap_top + i;
            size_t jj = (pen_x26_6 >> 6) + slot->bitmap_left + j;
            uint8_t alpha = slot->bitmap.buffer[i * slot->bitmap.pitch + j];
            (*render_pixel)(jj, ii, amber, alpha);
        }
    }
    pen_x26_6 += slot->advance.x;
    pen_y26_6 += slot->advance.y;
}

void text_render_chars(const char *text)
{

    for (const char *p = text; *p; p++)
        render_codepoint((uint32_t)*p);
}

void text_render_codepoints(const uint32_t *text)
{
    for (const uint32_t *p = text; *p; p++)
        render_codepoint(*p);
}
