#ifndef TEXT_included
#define TEXT_included

#include <stddef.h>
#include <stdint.h>

#include "pixmap.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern void init_text(void);

    extern void text_set_cursor(pixmap *, int x, int y);
    extern void text_render_chars(const char *chars);
    extern void text_render_codepoints(const uint32_t *cpoints);

#ifdef __cplusplus
}
#endif

#endif /* !TEXT_included */
