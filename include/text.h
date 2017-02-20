#ifndef TEXT_included
#define TEXT_included

#include "pixmap.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct text_pixmap {
        pixmap pixels;
        int    ascent;
        int    descent;
        int    line_height;
    } text_pixmap;

#ifdef __cplusplus
}
#endif

#endif /* !TEXT_included */
