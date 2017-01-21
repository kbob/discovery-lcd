#ifndef PIXMAP_included
#define PIXMAP_included

#include <unistd.h>

#include "pixfmts.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct pixmap {
        ssize_t   pitch;        // row pitch in bytes
        pixfmt    format;
        size_t    w, h;
        void     *pixels;
    } pixmap;

    extern ssize_t pixmap_pixel_pitch(const pixmap *);

    extern void *pixmap_pixel_address(const pixmap *, int x, int y);

#ifdef __cplusplus
}
#endif

#endif /* !PIXMAP_included */
