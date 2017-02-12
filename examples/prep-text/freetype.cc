// C++ language headers
#include <string>

// C/POSIX headers
#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>

// External library headers
#include <ft2build.h>
#include FT_FREETYPE_H

// Application headers
#include "spitmap.h"
#include "unicode.h"

#define CHECK(error) (assert(error == 0))

static const char *const font_directories[] = {
    "fonts",
#ifdef __APPLE__
    "~/Library/Fonts",
    "/Library/Fonts",
    "/Network/Library/Fonts",
    "/System/Library/Fonts",
#else
    #error "add your platform's font path here"
#endif
    NULL
};

struct string_metrics {
    double width;
    double height;
    double ascent;
    double descent;
};

static std::string find_font(const std::string& name)
{
    for (const char *const *dir = font_directories; *dir; dir++) {
        std::string path = *dir;
        if (path.substr(0, 2) == "~/") {
            const char *home = getenv ("HOME");
            if (!home)
                continue;       // -> next font directory
            path = home + path.substr(1);
        }
        path += std::string("/") + name + ".ttf";
        int fd = open(path.c_str(), O_RDONLY);
        if (fd != -1) {
            close(fd);
            return path;
        }
    }
    fprintf(stderr,
            "make-button-img: can't find font file %s.ttf\n",
            name.c_str());
    exit(1);
}

class string_glyph_iterator {

public:
    string_glyph_iterator(FT_Face face, const options *opts)
        : m_face(face),
          m_is_aligned(opts->is_aligned),
          m_is_hinting(opts->is_hinting),
          m_str((uint8_t *)opts->text),
          m_glyph_index(0),
          m_prev_gl_idx(0),
          m_pen_x(round(opts->translation * 64.0)),
          m_pen_y(0)
    {
        ++*this;                // step to first char
    }

    void operator ++()
    {
        FT_Error error;
        uint32_t codepoint;
        int bytes = decode_utf8_codepoint(m_str, &codepoint);
        assert(bytes >= 0);
        if (bytes == 0) {
            m_glyph_index = 0;
            return;
        }
        m_str += bytes;
        m_glyph_index = FT_Get_Char_Index(m_face, codepoint);
        if (m_prev_gl_idx) {
            FT_Vector kerning;
            FT_Kerning_Mode kerning_mode = (m_is_hinting
                                            ? FT_KERNING_DEFAULT
                                            : FT_KERNING_UNFITTED);
            error = FT_Get_Kerning(m_face,
                                   m_prev_gl_idx,
                                   m_glyph_index, 
                                   kerning_mode,
                                   &kerning);
            CHECK(error);
            m_pen_x += m_face->glyph->advance.x + kerning.x;
            m_pen_y += m_face->glyph->advance.y + kerning.y;
            m_prev_gl_idx = m_glyph_index;
        }
        if (!m_is_aligned) {
            FT_Vector translation = { m_pen_x & 0x3F, m_pen_y & 0x3F };
            FT_Set_Transform(m_face, NULL, &translation);
        }
        error = FT_Load_Glyph(m_face, m_glyph_index, FT_LOAD_DEFAULT);
        CHECK(error);
        error = FT_Render_Glyph(m_face->glyph, FT_RENDER_MODE_NORMAL);
        CHECK(error);
        m_prev_gl_idx = m_glyph_index;
    }

                 operator bool() const { return (bool)m_glyph_index;  }
    FT_F26Dot6   pen_x        () const { return m_pen_x;       }
    FT_F26Dot6   pen_y        () const { return m_pen_x;       }
    FT_GlyphSlot operator *   ()       { return m_face->glyph; }

private:
    void advance()
    {
    }

    FT_Face    m_face;
    bool       m_is_aligned;
    bool       m_is_hinting;
    uint8_t   *m_str;
    FT_UInt    m_glyph_index;
    FT_UInt    m_prev_gl_idx;
    FT_F26Dot6 m_pen_x;
    FT_F26Dot6 m_pen_y;

    // undefined
    string_glyph_iterator(const string_glyph_iterator&);
    void operator = (const string_glyph_iterator&);
};

static string_metrics collect_metrics(FT_Face face, const options *opts)
{
    string_metrics metrics = {0};

    for (string_glyph_iterator it(face, opts); it; ++it) {
        FT_GlyphSlot slot = *it;
        double width = ceil((it.pen_x() + slot->metrics.width) / 64.0) +
                       slot->bitmap_left;
        // printf("width = ceil((%ld + %ld) / 64) + %d = %g\n",
        //        it.pen_x(), slot->metrics.width, slot->bitmap_left, width);
        double height = face->height / 64.0;
        double ascent = slot->bitmap_top;
        double descent = slot->bitmap.rows - ascent;
        if (metrics.width < width)
            metrics.width = width;
        if (metrics.height < height)
            metrics.height = height;
        if (metrics.ascent < ascent)
            metrics.ascent = ascent;
        if (metrics.descent < descent)
            metrics.descent = descent;
    }
    // printf("metrics = {\n");
    // printf("    .width   = %g,\n", metrics.width);
    // printf("    .height  = %g,\n", metrics.height);
    // printf("    .ascent  = %g,\n", metrics.ascent);
    // printf("    .descent = %g,\n", metrics.descent);
    // printf("};\n\n");
    return metrics;
}

static void render_string(graymap *pixels,
                          FT_Face face,
                          const string_metrics *metrics,
                          const options *opts)
{
    size_t baseline = ceil(metrics->ascent);
    for (string_glyph_iterator it(face, opts); it; ++it) {
        FT_GlyphSlot slot = *it;
        for (size_t i = 0; i < slot->bitmap.rows; i++) {
            size_t y = i + baseline - slot->bitmap_top;
            for (size_t j = 0; j < slot->bitmap.width; j++) {
                size_t x = j + round(it.pen_x() / 64.0) + slot->bitmap_left;
                // if (x >= pixels->w) {
                //     printf("(H, W) = (%lu, %lu)\n", pixels->h, pixels->w);
                //     printf("x = %zu + round(%ld/64) + %d = %zu\n",
                //            j, it.pen_x(), slot->bitmap_left, x);
                // }
                assert(x < pixels->w);
                assert(y < pixels->h);
                uint8_t *pix = pixels->pixels + y * pixels->pitch + x;
                uint8_t alpha = slot->bitmap.buffer[i * slot->bitmap.pitch + j];
                *pix = alpha;
            }
        }
    }
}

static graymap *render_face(FT_Face face, const options *opts)
{
    string_metrics metrics = collect_metrics(face, opts);
    size_t width = ceil(metrics.width);
    size_t height = ceil(metrics.ascent) + ceil(metrics.descent);
    graymap *pixels = alloc_graymap(width, height);
    render_string(pixels, face, &metrics, opts);
    return pixels;
}

graymap *render_freetype(const options *opts)
{
    assert(opts->renderer == R_FREETYPE);
    assert(!opts->is_color);
    assert(opts->is_hinting);
    assert(opts->font);

    FT_Library library;
    FT_Face face;

    FT_Error error = FT_Init_FreeType(&library);
    CHECK(error);

    std::string font_file = find_font(opts->font);
    error = FT_New_Face(library, font_file.c_str(), 0, &face);
    CHECK(error);

    FT_F26Dot6 char_size = round(opts->size * 64.0);
    FT_F26Dot6 resolution = round(opts->resolution);
    error = FT_Set_Char_Size(face, 0, char_size, resolution, resolution);
    CHECK(error);

    graymap *pixels = render_face(face, opts);
    
    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return pixels;
}
