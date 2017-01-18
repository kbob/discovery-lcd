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

#define FONT_SIZE (10 * 64)

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

#else
static const char message[] = "Squdgy fez, blank jimp crwth vox! illliill 11111111";
#endif

static struct FT_MemoryRec_ memory;
static FT_Library library;
static FT_Face face;
static bool ft_is_initialized;

#define CHECK(error) (assert(error == 0))

extern uint8_t _binary_examples_text_Lato_Light_ttf_start[];
extern uint8_t _binary_examples_text_Lato_Light_ttf_size[];

static const int start_pen_x = 100 << 6;
static const int start_pen_y = 100 << 6;

volatile struct {
    uint32_t alloc_blocks;
    uint32_t alloc_bytes;
    uint32_t freed_blocks;
    uint32_t freed_bytes;
    uint32_t realloc_calls;
    uint32_t realloc_delta;
    uint32_t realloc_growth;
    uint32_t realloc_shrinkage;
} mem_stats;

static void *my_alloc(FT_Memory mem, long size)
{
    (void)mem;
    mem_stats.alloc_blocks++;
    mem_stats.alloc_bytes += size;
    return malloc(size);
}

static void *my_realloc(FT_Memory mem,
                        long      cur_size,
                        long      new_size,
                        void     *block)
{
    (void)mem;
    (void)cur_size;
    mem_stats.realloc_calls++;
    if (new_size > cur_size)
        mem_stats.realloc_growth += new_size - cur_size;
    else
        mem_stats.realloc_shrinkage += cur_size - new_size;
    mem_stats.realloc_delta += new_size - cur_size;
    return realloc(block, new_size);
}

static void my_free(FT_Memory mem, void *block)
{
    (void)mem;
    mem_stats.freed_blocks++;
    free(block);
}

void init_text(void)
{
#if 0
    FT_Error error = FT_Init_FreeType(&library);
    CHECK(error);
#else
    memory.user    = NULL;
    memory.alloc   = my_alloc;
    memory.realloc = my_realloc;
    memory.free    = my_free;
    FT_Error error = FT_New_Library(&memory, &library);
    CHECK(error);
    FT_Add_Default_Modules(library);
    CHECK(error);
#endif
    error = FT_New_Memory_Face(library,
                               _binary_examples_text_Lato_Light_ttf_start,
                               (FT_Long)_binary_examples_text_Lato_Light_ttf_size,
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

static rgb565 blend_pixel(xrgb_888 front, rgb565 back, uint8_t alpha)
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

volatile uint32_t pixel_bytes;

void render_text(rgb565 *data, size_t width, size_t height)
{
    (void)height;
    rgb565 (*pixels)[width] = (rgb565 (*)[width])data;

    if (!ft_is_initialized)
        return;

    FT_GlyphSlot slot = face->glyph;
    int pen_x = start_pen_x, pen_y = start_pen_y;

   for (typeof (*message) *p = message; *p; p++) {
       FT_ULong c = *p;
    // for (FT_ULong c = ' '; c < '\177'; c++) {
        FT_UInt glyph_index = FT_Get_Char_Index(face, c);
        assert(glyph_index);
        if (glyph_index == 0)
            continue;
        FT_Error error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
        CHECK(error);
        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        CHECK(error);
        pixel_bytes += slot->bitmap.width * slot->bitmap.rows;
        for (size_t i = 0; i < slot->bitmap.rows; i++) {
            for (size_t j = 0; j < slot->bitmap.width; j++) {
                size_t ii = (pen_y >> 6) - slot->bitmap_top + i;
                size_t jj = (pen_x >> 6) + slot->bitmap_left + j;
                rgb565 *pix = &pixels[ii][jj];
                uint8_t alpha = slot->bitmap.buffer[i * slot->bitmap.pitch + j];
                *pix = blend_pixel(amber, *pix, alpha);
            }
        }
        FT_Vector kerning = { 0, 0 };
        FT_ULong next_c = c + 1;
        if (next_c) {
            FT_UInt next_glyph = FT_Get_Char_Index(face, next_c);
            error = FT_Get_Kerning(face,
                                   glyph_index,
                                   next_glyph,
                                   FT_KERNING_DEFAULT,
                                   &kerning);
            CHECK(error);
        }
        pen_x += slot->advance.x + kerning.x;
        pen_y += slot->advance.y + kerning.y;
        if ((c & 31) == 31) {
            pen_x = start_pen_x;
            pen_y += slot->metrics.vertAdvance;
        }
    }
}
