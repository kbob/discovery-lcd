#ifndef SPITMAP_included
#define SPITMAP_included

#include <stddef.h>

#include "graymap.h"

typedef enum visual {
    V_ARGB8888,
    V_RGB888,
    V_RGB565,
    V_ARGB1555,
    V_ARGB4444,
    V_AL88,
    V_L8,
    V_A8,
    V_AL44,
    V_A4,
    V_L4,
} visual;

typedef enum renderer {
    R_FREETYPE,
    R_AGG,
} renderer;

struct options {
    bool        is_aligned;
    bool        is_color;
    bool        is_hinting;
    const char *font;
    renderer    renderer;
    double      resolution;
    double      size;
    double      translation;
    visual      visual;
    const char *identifier;
    const char *text;
    const char *out_file;

    options();
};

extern const char *progname;

extern graymap *render_freetype(const options *);
extern graymap *render_agg(const options *);

#endif /* !SPITMAP_included */
