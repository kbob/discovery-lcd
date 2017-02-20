#include "output.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

static const char *renderer_name(renderer r)
{
    switch (r) {

    case R_FREETYPE:
        return "freetype";

    case R_AGG:
        return "antigrain";

    default:
        assert(false);
    }
}

static const char *visual_name(visual v)
{
    return "a8";                // XXX
}

static const char *visual_enum(visual v)
{
    return "PF_A8";             // XXX
}

static int visual_size(visual v)
{
    return 1;                   // XXX
}

__attribute__((format(printf, 2, 3)))
void fpcomment(FILE *out, const char *fmt, ...)
{
    const size_t max_width = 72 - 6;
    char buffer[max_width + 1];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buffer, sizeof buffer, fmt, ap);
    va_end(ap);
    assert(n <= max_width);
    fprintf(out, "/* %-*s */\n", (int)max_width, buffer);
}

void output_graymap(const graymap *map, const options *opts)
{
    FILE *out = stdout;
    if (opts->out_file) {
        out = fopen(opts->out_file, "w");
        if (!out) {
            perror(opts->out_file);
            exit(1);
        }
    }

    const char *ident     = opts->identifier;
    const char *type_name = visual_name(opts->visual);
    const char *type_enum = visual_enum(opts->visual);
    const int pitch       = map->w * visual_size(opts->visual);

    // Make a comment block to describe this file.
    fpcomment(out, "This file automatically created by %s.", progname);
    fpcomment(out, "Do not edit.");
    fpcomment(out, "");
    fpcomment(out, "Parameters:");
    fpcomment(out, "    identifier  = %s", ident);
    fpcomment(out, "    text        = \"%s\"", opts->text);
    fpcomment(out, "    font        = %s", opts->font);
    fpcomment(out, "    size        = %g", opts->size);
    fpcomment(out, "    renderer    = %s", renderer_name(opts->renderer));
    fpcomment(out, "    resolution  = %g", opts->resolution);
    fpcomment(out, "    aligned     = %s", opts->is_aligned ? "true" : "false");
    fpcomment(out, "    color       = %s", opts->is_color   ? "true" : "false");
    fpcomment(out, "    hinting     = %s", opts->is_hinting ? "true" : "false");
    fpcomment(out, "    translation = %g", opts->translation);
    fpcomment(out, "    visual      = %s (%s)", type_name, type_enum);
    fprintf(out, "\n");

    fprintf(out, "#include \"text.h\"\n");
    fprintf(out, "\n");
    fprintf(out, "static const %s %s_bits[] = {\n", type_name, ident);
    int col = 0;
    for (size_t i = 0; i < map->h; i++) {
        for (size_t j = 0; j < map->w; j++) {
            uint8_t pix = map->pixels[i * map->pitch + j];
            char buf[8];
            int nc = snprintf(buf, sizeof buf, "%#4x,", pix);
            if ((i && !j) || col + nc > 72) {
                fprintf(out, "\n");
                col = 0;
            }
            if (col == 0) {
                fprintf(out, "    ");
                col = 4;
            } else {
                fprintf(out, " ");
                col++;
            }
            fprintf(out, "%s", buf);
            col += nc;
        }
        fprintf(out, "\n");
        col = 0;
    }
    fprintf(out, "};\n");
    fprintf(out, "\n");
    fprintf(out, "const text_pixmap %s = {\n", ident);
    fprintf(out, "    .pixels = {\n");
    fprintf(out, "        .pitch   = %d,\n", pitch);
    fprintf(out, "        .format  = %s,\n", type_enum);
    fprintf(out, "        .w       = %zu,\n", map->w);
    fprintf(out, "        .h       = %zu,\n", map->h);
    fprintf(out, "        .pixels  = (void *)%s_bits,\n", ident);
    fprintf(out, "    },\n");
    fprintf(out, "    .ascent      = %d,\n", map->ascent);
    fprintf(out, "    .descent     = %d,\n", map->descent);
    fprintf(out, "    .line_height = %d,\n", map->line_height);
    fprintf(out, "};\n");
}
