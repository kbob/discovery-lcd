#ifndef PIXFMTS_included
#define PIXFMTS_included

typedef enum pixfmt {
    PF_NONE = 0,
    PF_ARGB8888,
    PF_RGB888,
    PF_RGB565,
    PF_ARGB1555,
    PF_ARGB4444,
    PF_L8,
    PF_AL44,
    PF_AL88,
    PF_COUNT                    // must be last
} pixfmt;

typedef uint32_t argb8888;
typedef uint32_t xrgb_888;       // rgb888 padded to 32 bits
typedef struct rgb888 {
    uint8_t r, g, b;
} rgb888;
typedef uint16_t rgb565;
typedef uint16_t argb1555;
typedef uint16_t rgba4444;
typedef uint8_t  l8;
typedef uint8_t  al44;
typedef uint16_t al88;


#endif /* !PIXFMTS_included */
