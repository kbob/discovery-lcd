#ifndef TEXT_included
#define TEXT_included

#include <stddef.h>
#include <stdint.h>

#include "pixfmts.h"

extern void init_text(void);

extern void render_text(void *pixels,
                        pixfmt format,
                        size_t width,
                        size_t height);

#endif /* !TEXT_included */
