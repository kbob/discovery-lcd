#ifndef GRAYMAP_included
#define GRAYMAP_included

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

struct graymap {
    uint8_t *pixels;
    size_t w, h;
    int ascent, descent;
    ssize_t pitch;
};

extern graymap *alloc_graymap(size_t w, size_t h);

extern void free_graymap(graymap *);

# endif /* !GRAYMAP_included */
