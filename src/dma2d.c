#include "dma2d.h"

#include <assert.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma2d.h>
#include <libopencm3/stm32/rcc.h>

#include "intr.h"

static dma2d_request *request_queue;
static volatile size_t rq_size;
static volatile size_t rq_head;
static volatile size_t rq_tail;
static volatile bool dma_busy;

#define CHECK_DEST_FORMAT(dst)                                          \
    (assert(dest->format == PF_ARGB8888 ||                              \
            dest->format == PF_RGB888   ||                              \
            dest->format == PF_RGB565   ||                              \
            dest->format == PF_ARGB1555 ||                              \
            dest->format == PF_ARGB4444))

#define CHECK_SRC_FORMAT(src)                                           \
    (assert(src->format  == PF_ARGB8888 ||                              \
            src->format  == PF_RGB888   ||                              \
            src->format  == PF_RGB565   ||                              \
            src->format  == PF_ARGB1555 ||                              \
            src->format  == PF_ARGB4444 ||                              \
            src->format  == PF_L8       ||                              \
            src->format  == PF_AL44     ||                              \
            src->format  == PF_AL88     ||                              \
            src->format  == PF_L4       ||                              \
            src->format  == PF_A8       ||                              \
            src->format  == PF_A4))

void init_dma2d(dma2d_request *queue, size_t count)
{
    assert(!request_queue);
    request_queue = queue;
    rq_size = count;
    rq_head = 0;
    rq_tail = 0;

    rcc_periph_clock_enable(RCC_DMA2D);
    nvic_enable_irq(NVIC_DMA2D_IRQ);
}

bool dma2d_queue_is_full(void)
{
    assert(request_queue);
    size_t h, t, s;
    WITH_INTERRUPTS_MASKED {
        h = rq_head;
        t = rq_tail;
        s = rq_size;
    }
    return h == (t + 1) % s;
}

bool dma2d_queue_is_empty(void)
{
    assert(request_queue);
    size_t h, t;
    WITH_INTERRUPTS_MASKED {
        h = rq_head;
        t = rq_tail;
    }
    return h == t;
}

bool dma2d_is_idle(void)
{
    bool busy;
    WITH_INTERRUPTS_MASKED {
        busy = dma_busy;
    }
    return !busy;
}

static dma2d_request *dequeue_request(void)
{
    assert(!dma2d_queue_is_empty());
    dma2d_request *req = &request_queue[rq_head];
    rq_head = (rq_head + 1) % rq_size;
    return req;
}

static uint32_t dma2d_dest_color_mode(pixfmt format)
{
    switch (format) {

    case PF_ARGB8888:
        return 0b000 << DMA2D_OPFCCR_CM_SHIFT;

    case PF_RGB888:
        return 0b001 << DMA2D_OPFCCR_CM_SHIFT;

    case PF_RGB565:
        return 0b010 << DMA2D_OPFCCR_CM_SHIFT;

    case PF_ARGB1555:
        return 0b011 << DMA2D_OPFCCR_CM_SHIFT;

    case PF_ARGB4444:
        return 0b100 << DMA2D_OPFCCR_CM_SHIFT;

    default:
        assert(false);
    }
}

static void start_solid_request(dma2d_request *req)
{
    assert(req->type == DRT_SOLID);

    DMA2D_OPFCCR = dma2d_dest_color_mode(req->dest.format);
    DMA2D_OCOLR  = req->solid.color;
    DMA2D_OMAR   = (uint32_t)req->dest.pixels;
    DMA2D_OOR    = pixmap_pixel_pitch(&req->dest) - req->dest.w;
    DMA2D_NLR    = (req->dest.w << DMA2D_NLR_PL_SHIFT |
                    req->dest.h << DMA2D_NLR_NL_SHIFT);

    DMA2D_IFCR   = DMA2D_IFCR_CCEIF | DMA2D_IFCR_CTCIF | DMA2D_IFCR_CTEIF;

    DMA2D_CR     = (DMA2D_CR_MODE_R2M << DMA2D_CR_MODE_SHIFT |
                    DMA2D_CR_CEIE |
                    DMA2D_CR_TCIE |
                    DMA2D_CR_TEIE |
                    DMA2D_CR_START);
}

static void start_request(void)
{
    dma2d_request *req;
    WITH_INTERRUPTS_MASKED {
        req = &request_queue[rq_head];
    }

    switch (req->type) {

    case DRT_SOLID:
        start_solid_request(req);
        break;

    case DRT_COPY:
        // start_copy_request(req);
        break;

    case DRT_PFC:
        // start_pfc_request(req);
        break;

    case DRT_BLEND:
        // start_blend_request(req);
        break;

    case DRT_CLUT:
        // start_clut_request(req);
        break;
    }
}

static dma2d_request *alloc_request(void)
{
    while (dma2d_queue_is_full())
        continue;

    dma2d_request *req;
    WITH_INTERRUPTS_MASKED {
        req = &request_queue[rq_tail];
    }
    return req;
}

static void enqueue_request(dma2d_request *req)
{
    bool busy;
    WITH_INTERRUPTS_MASKED {
        assert(req == &request_queue[rq_tail]);
        rq_tail = (rq_tail + 1) % rq_size;
        busy = dma_busy;
        if (!busy)
            dma_busy = true;
    }
    if (!busy)
        start_request();
}

void dma2d_enqueue_solid_request(pixmap *dest,
                                 uint32_t color,
                                 dma2d_callback *cb)
{
    CHECK_DEST_FORMAT(dest);
    dma2d_request *req  = alloc_request();

    req->type           = DRT_SOLID;
    req->callback       = cb;
    req->dest           = *dest;
    req->solid.color    = color;

    enqueue_request(req);
}

void dma2d_enqueue_copy_request(pixmap *dest,
                                pixmap *src,
                                dma2d_callback *cb)
{
    CHECK_DEST_FORMAT(dest);
    assert(src->format == dest->format);
    dma2d_request *req  = alloc_request();

    req->type           = DRT_COPY;
    req->callback       = cb;
    req->dest           = *dest;
    req->copy.src       = *src;

    enqueue_request(req);
}

void dma2d_enqueue_pfc_request(pixmap *dest,
                               pixmap *src,
                               xrgb_888 src_color,
                               bool src_uses_clut,
                               dma2d_alpha_mode src_alpha_mode,
                               uint8_t src_alpha,
                               dma2d_callback *cb)
{
    CHECK_DEST_FORMAT(dest);
    CHECK_SRC_FORMAT(src);
    dma2d_request *req = alloc_request();

    req->type               = DRT_PFC;
    req->callback           = cb;
    req->dest               = *dest;
    req->pfc.src            = *src;
    req->pfc.src_color      = src_color;
    req->pfc.src_uses_clut  = src_uses_clut;
    req->pfc.src_alpha_mode = src_alpha_mode;
    req->pfc.src_alpha      = src_alpha;

    enqueue_request(req);
}

void dma2d_enqeue_blend_request(pixmap *dest,
                                pixmap *fg,
                                pixmap *bg,
                                xrgb_888 fg_color,
                                xrgb_888 bg_color,
                                bool fg_uses_clut,
                                bool bg_uses_clut,
                                dma2d_alpha_mode fg_alpha_mode,
                                dma2d_alpha_mode bg_alpha_mode,
                                uint8_t fg_alpha,
                                uint8_t bg_alpha,
                                dma2d_callback *cb)
{
    CHECK_DEST_FORMAT(dest);
    CHECK_SRC_FORMAT(fg);
    CHECK_SRC_FORMAT(bg);
    dma2d_request *req = alloc_request();

    req->type                = DRT_BLEND;
    req->callback            = cb;
    req->dest                = *dest;
    req->blend.fg            = *fg;
    req->blend.bg            = *bg;
    req->blend.fg_color      = fg_color;
    req->blend.bg_color      = bg_color;
    req->blend.fg_uses_clut  = fg_uses_clut;
    req->blend.bg_uses_clut  = bg_uses_clut;
    req->blend.fg_alpha_mode = fg_alpha_mode;
    req->blend.bg_alpha_mode = bg_alpha_mode;
    req->blend.fg_alpha      = fg_alpha;
    req->blend.bg_alpha      = bg_alpha;

    enqueue_request(req);
}


void dma2d_isr(void)
{
    uint32_t isr = DMA2D_ISR;
    DMA2D_IFCR = (DMA2D_IFCR_CCEIF  |
                  DMA2D_IFCR_CCTCIF |
                  DMA2D_IFCR_CCAEIF |
                  DMA2D_IFCR_CTWIF  |
                  DMA2D_IFCR_CTCIF  |
                  DMA2D_IFCR_CTEIF);
    assert(!(isr & (DMA2D_ISR_CEIF | DMA2D_ISR_CAEIF | DMA2D_ISR_TEIF)));
    
    if (isr & (DMA2D_ISR_CTCIF | DMA2D_ISR_TCIF)) {
        dma2d_request *req = dequeue_request();
        if (req->callback)
            (*req->callback)(req);

        if (rq_head == rq_tail) {
            // request queue is empty.
            dma_busy = false;
        } else
            start_request();
    }
}
