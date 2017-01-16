#ifndef TEXT_included
#define TEXT_included

#include <stddef.h>
#include <stdint.h>

#include "pixfmts.h"

extern void init_text(void);

extern void render_text(rgb565 *pixels, size_t width, size_t height);

#endif /* !TEXT_included */
