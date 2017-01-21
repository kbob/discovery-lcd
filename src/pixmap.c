#include "pixmap.h"

#include <assert.h>
#include <stdbool.h>

int pixmap_pixels_to_bytes(const pixmap *p, int pixels);
int pixmap_pixels_to_bytes(const pixmap *p, int pixels)
{
    switch (p->format) {
    case PF_ARGB8888:
        return pixels * 4;

    case PF_RGB888:
        return pixels * 3;

    case PF_RGB565:
    case PF_ARGB1555:
    case PF_ARGB4444:
    case PF_AL88:
        return pixels * 2;

    case PF_L8:
    case PF_AL44:
    case PF_A8:
        return pixels;

    case PF_L4:
    case PF_A4:
        return pixels / 2;

    case PF_NONE:
    case PF_COUNT:
        assert(false);
    }
    return 0;
}

int pixmap_bytes_to_pixels(const pixmap *p, int bytes);
int pixmap_bytes_to_pixels(const pixmap *p, int bytes)
{
    switch (p->format) {
    case PF_ARGB8888:
        return bytes / 4;

    case PF_RGB888:
        return bytes / 3;

    case PF_RGB565:
    case PF_ARGB1555:
    case PF_ARGB4444:
    case PF_AL88:
        return bytes / 2;

    case PF_L8:
    case PF_AL44:
    case PF_A8:
        return bytes;

    case PF_L4:
    case PF_A4:
        return bytes * 2;

    case PF_NONE:
    case PF_COUNT:
        assert(false);
    }
    return 0;
}

ssize_t pixmap_pixel_pitch(const pixmap *p)
{
    return pixmap_bytes_to_pixels(p, p->pitch);
    // ssize_t pitch = p->pitch;

    // switch (p->format) {
    // case PF_ARGB8888:
    //     return pitch * 4;

    // case PF_RGB888:
    //     return pitch * 3;

    // case PF_RGB565:
    // case PF_ARGB1555:
    // case PF_ARGB4444:
    // case PF_AL88:
    //     return pitch * 2;

    // case PF_L8:
    // case PF_AL44:
    // case PF_A8:
    //     return pitch;

    // case PF_L4:
    // case PF_A4:
    //     return pitch / 2;

    // case PF_NONE:
    // case PF_COUNT:
    //     assert(false);
    // }
    // return 0;
}

void *pixmap_pixel_address(const pixmap *p, int x, int y)
{
    return p->pixels + y * p->pitch + pixmap_pixels_to_bytes(p, x);
}

