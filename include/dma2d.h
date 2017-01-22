#ifndef DMA2D_included
#define DMA2D_included

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "pixmap.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum dma2d_request_type {
        DRT_SOLID,
        DRT_COPY,
        DRT_PFC,
        DRT_BLEND,
        DRT_CLUT,
    } dma2d_request_type;

    typedef enum dma2d_alpha_mode {
        DAM_SRC,                // use alpha from source pixel
        DAM_REQ,                // use alpha from request
        DAM_PRODUCT,            // use src_alpha * request_alpha
    } dma2d_alpha_mode;
 
    typedef struct dma2d_request dma2d_request;

    typedef struct dma2d_solid_req {
        uint32_t            color;
    } dma2d_solid_req;

    typedef struct dma2d_copy_req {
        pixmap              src;
    } dma2d_copy_req;

    typedef struct dma2d_pfc_req { // pixel format conversion
        pixmap              src;
        xrgb_888            src_color;
        dma2d_alpha_mode    src_alpha_mode;
        uint8_t             src_alpha;
    } dma2d_pfc_req;

    typedef struct dma2d_blend_req {
        pixmap              fg;
        pixmap              bg;
        xrgb_888            fg_color;
        xrgb_888            bg_color;
        dma2d_alpha_mode    fg_alpha_mode: 2;
        dma2d_alpha_mode    bg_alpha_mode: 2;
        uint8_t             fg_alpha;
        uint8_t             bg_alpha;
    } dma2d_blend_req;

    typedef struct dma2d_load_clut_req {
        bool                is_bg;
        bool                has_alpha;
        uint8_t             clut_len;
        union {
            rgb888         *clut_rgb;
            argb8888       *clut_argb;
        };
    } dma2d_load_clut_req;

    typedef void dma2d_callback(dma2d_request *);

    struct dma2d_request {
        dma2d_request_type  type;
        dma2d_callback     *callback;
        pixmap              dest;
        union {
            dma2d_solid_req solid;
            dma2d_copy_req  copy;
            dma2d_pfc_req   pfc;
            dma2d_blend_req blend;
        };
    };

    // The app allocates the DMA2D request queue.  The app decides
    // how big it should be.
    extern void init_dma2d(dma2d_request *queue, size_t count);

    extern bool dma2d_queue_is_full(void);
    extern bool dma2d_queue_is_empty(void);
    extern bool dma2d_is_idle(void);

    // Fill destination pixmap with a single pixel value.
    // Pixel value should match destination pixmap's format.
    extern void dma2d_enqueue_solid_request(pixmap *dest,
                                            uint32_t color,
                                            dma2d_callback *);

    // Copy src to dest.  Use dest's size and pixel format.
    // No pixel format conversion done.
    extern void dma2d_enqueue_copy_request(pixmap *dest,
                                           pixmap *src,
                                           dma2d_callback *);

    // Copy src to dest using dest's size.
    // Pixels are converted from src's format to dest's,
    // and alpha may be modified.
    extern void dma2d_enqueue_pfc_request(pixmap *dest,
                                          pixmap *src,
                                          xrgb_888 src_color,
                                          dma2d_alpha_mode src_alpha_mode,
                                          uint8_t src_alpha,
                                          dma2d_callback *);

    // Blend foreground and background pixmaps; store result
    // in dest using dest's size.  fg and bg alpha values may be
    // modified before blending.
    extern void dma2d_enqueue_blend_request(pixmap *dest,
                                            pixmap *fg,
                                            pixmap *bg,
                                            xrgb_888 fg_color,
                                            xrgb_888 bg_color,
                                            dma2d_alpha_mode fg_alpha_mode,
                                            dma2d_alpha_mode bg_alpha_mode,
                                            uint8_t fg_alpha,
                                            uint8_t bg_alpha,
                                            dma2d_callback *);
                                           
    // Load fg or bg CLUT.
    extern void dma2d_enqueue_clut_request(bool is_bg,
                                           bool has_alpha,
                                           void *clut,
                                           size_t clut_len,
                                           dma2d_callback *);


#ifdef __cplusplus
}
#endif

#endif /* !DMA2D_included */
