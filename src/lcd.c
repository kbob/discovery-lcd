#include "lcd.h"

#include <assert.h>

#include <libopencm3/cm3/nvic.h>
#define rgb888 libopencm3_rgb888 // XXX Yuck!
   #include <libopencm3/stm32/ltdc.h>
#undef rgb888
#include <libopencm3/stm32/rcc.h>

#include "gpio.h"
#include "systick.h"

const lcd_config discovery_lcd = {
    .width         = 240,
    .height        = 320,
    {
        .sain      = 192,
        .sair      = 4,
    },
    .h_sync        = 10,
    .h_back_porch  = 20,
    .h_front_porch = 10,
    .v_sync        = 2,
    .v_back_porch  = 2,
    .v_front_porch = 2,
    .use_dither    = true,
};

const lcd_config adafruit_p1596_lcd = {
    .width         = 800,
    .height        = 480,
    {
        .sain      = 180,
        .sair      = 90,
    },
    .h_sync        = 48,
    .h_back_porch  = 88,
    .h_front_porch = 48,
    .v_sync        = 3,
    .v_back_porch  = 32,
    .v_front_porch = 13,
    .use_dither    = true,
};

static const gpio_pin gpio_pins[] = {
    {
        .gp_port   = GPIOA,
        .gp_pin    = GPIO3 | GPIO4 | GPIO6 | GPIO11 | GPIO12,
        .gp_mode   = GPIO_MODE_AF,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {
        .gp_port = GPIOB,
        .gp_pin    = GPIO0 | GPIO1,
        .gp_mode   = GPIO_MODE_AF,
        .gp_af     = GPIO_AF9,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {
        .gp_port   = GPIOB,
        .gp_pin    = GPIO8 | GPIO9 | GPIO10 | GPIO11,
        .gp_mode   = GPIO_MODE_AF,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {
        .gp_port   = GPIOC,
        .gp_pin    = GPIO6 | GPIO7 | GPIO10,
        .gp_mode   = GPIO_MODE_AF,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {
        .gp_port   = GPIOD,
        .gp_pin    = GPIO3 | GPIO6,
        .gp_mode   = GPIO_MODE_AF,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {
        .gp_port   = GPIOF,
        .gp_pin    = GPIO10,
        .gp_mode   = GPIO_MODE_AF,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {
        .gp_port   = GPIOG,
        .gp_pin    = GPIO6 | GPIO7 | GPIO11,
        .gp_mode   = GPIO_MODE_AF,
        .gp_af     = GPIO_AF14,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
    {
        .gp_port   = GPIOG,
        .gp_pin    = GPIO10 | GPIO12,
        .gp_mode   = GPIO_MODE_AF,
        .gp_af     = GPIO_AF9,
        .gp_ospeed = GPIO_OSPEED_50MHZ,
    },
};
static const size_t gpio_pin_count = (&gpio_pins)[1] - gpio_pins;

static lcd_config current_config;
// Default settings is black screen, neither layer enabled.
static lcd_settings current_settings;
static lcd_settings_callback *frame_callback;
static lcd_settings_callback *line_callback;

static uint32_t ltdc_pixfmt(pixfmt f)
{
    switch (f) {

    case PF_ARGB8888:
        return LTDC_LxPFCR_ARGB8888;

    case PF_RGB888:
        return LTDC_LxPFCR_RGB888;

    case PF_RGB565:
        return LTDC_LxPFCR_RGB565;

    case PF_ARGB1555:
        return LTDC_LxPFCR_ARGB1555;

    case PF_ARGB4444:
        return LTDC_LxPFCR_ARGB4444;

    case PF_L8:
        return LTDC_LxPFCR_L8;

    case PF_AL44:
        return LTDC_LxPFCR_AL44;

    case PF_AL88:
        return LTDC_LxPFCR_AL88;

    default:
        assert(0);
    }
}

static size_t pixfmt_size_bytes(pixfmt f)
{
    switch (f) {

    case PF_ARGB8888:
        return 4;

    case PF_RGB888:
        return 3;

    case PF_RGB565:
        return 2;

    case PF_ARGB1555:
        return 2;

    case PF_ARGB4444:
        return 2;

    case PF_L8:
        return 1;

    case PF_AL44:
        return 1;

    case PF_AL88:
        return 2;

    default:
        assert(0);
    }
}

static size_t pixfmt_clut_size(pixfmt f)
{
    switch (f) {

    case PF_L8:
        return 256;

    case PF_AL44:
        return 16;

    case PF_AL88:
        return 256;

    default:
        return 0;
    }
}

static void lcd_load_layer_settings(int layer, const lcd_layer_settings *ls)
{
    if (ls->is_enabled) {

        // Clip position and size to active window.
        int x = ls->position.x;
        int y = ls->position.y;
        int w = ls->pixels.w;
        int h = ls->pixels.h;
        intptr_t pix_addr = (intptr_t)ls->pixels.pixels;
        if (x < 0) {            // clip left
            w -= -x;
            pix_addr += -x * pixfmt_size_bytes(ls->pixels.format);
            x = 0;
        }
        if (x + w >= current_config.width) // clip right
            w = current_config.width - x;
        if (y < 0) {            // clip top
            h -= -y;
            pix_addr += -y * ls->pixels.pitch;
            y = 0;
        }
        if (y + h >= current_config.height) // clip bottom
            h = current_config.height - y;

        // Load position.
        uint32_t whsppos = (current_config.h_sync +
                            current_config.h_back_porch +
                            x + w - 1);
        uint32_t whstpos = (current_config.h_sync +
                            current_config.h_back_porch +
                            x);
        uint32_t wvsppos = (current_config.v_sync +
                            current_config.v_back_porch +
                            y + h - 1);
        uint32_t wvstpos = (current_config.v_sync +
                            current_config.v_back_porch +
                            y);
        LTDC_LxWHPCR(layer) = (whsppos << LTDC_LxWHPCR_WHSPPOS_SHIFT |
                               whstpos << LTDC_LxWHPCR_WHSTPOS_SHIFT);
        LTDC_LxWVPCR(layer) = (wvsppos << LTDC_LxWVPCR_WVSPPOS_SHIFT |
                               wvstpos << LTDC_LxWVPCR_WVSTPOS_SHIFT);

        // Set pixel format.
        LTDC_LxPFCR(layer) = ltdc_pixfmt(ls->pixels.format);

        // Set frame buffer address.
        LTDC_LxCFBAR(layer) = pix_addr;

        // Set line length and pitch.
        uint32_t cfbp = ls->pixels.pitch;
        uint32_t cfbll = w * pixfmt_size_bytes(ls->pixels.format) + 3;
        LTDC_LxCFBLR(layer) = (cfbp << LTDC_LxCFBLR_CFBP_SHIFT |
                               cfbll << LTDC_LxCFBLR_CFBLL_SHIFT);

        // Set number of lines.
        LTDC_LxCFBLNR(layer) = h << LTDC_LxCFBLNR_CFBLNBR_SHIFT;

        // Load CLUT.
        if (ls->clut)
        {
            size_t clut_count = pixfmt_clut_size(ls->pixels.format);
            for (size_t i = 0; i < clut_count; i++)
                LTDC_LxCLUTWR(layer) =
                    i << 24 | ((*ls->clut).c256[i] & 0x00FFFFFF);
        }

        // Set default color and blending factors.
        uint32_t bf1, bf2;
        if (ls->uses_pixel_alpha)
            bf1 = LTDC_LxBFCR_BF1_PIXEL_ALPHA_x_CONST_ALPHA;
        else
            bf1 = LTDC_LxBFCR_BF1_CONST_ALPHA;
        if (ls->uses_subjacent_alpha)
            bf2 = LTDC_LxBFCR_BF2_PIXEL_ALPHA_x_CONST_ALPHA;
        else
            bf2 = LTDC_LxBFCR_BF2_CONST_ALPHA;
        LTDC_LxDCCR(layer) = ls->default_pixel;
        LTDC_LxBFCR(layer) = (bf1 << 8 | bf2 << 0);
        LTDC_LxCACR(layer) = ls->alpha;

        // Set color key.
        LTDC_LxCKCR(layer) = ls->color_key_pixel;

        // Enable layer and CLUT.
        uint32_t cluten = ls->clut ? LTDC_LxCR_COLTAB_ENABLE : 0;
        uint32_t colken = ls->uses_color_key ? LTDC_LxCR_COLKEY_ENABLE : 0;
        uint32_t len = LTDC_LxCR_LAYER_ENABLE;
        LTDC_LxCR(layer) = cluten | colken | len;

    } else {

        // Disable layer.
        LTDC_LxCR(layer) = 0;
        LTDC_LxBFCR(layer) = (LTDC_LxBFCR_BF1_PIXEL_ALPHA_x_CONST_ALPHA |
                              LTDC_LxBFCR_BF2_PIXEL_ALPHA_x_CONST_ALPHA);
    }
}

void lcd_load_settings(const lcd_settings *settings, bool immediate)
{
    // Set background color.
    LTDC_BCCR = settings->bg_pixel;

    // Set line interrupt;
    if (line_callback)
        LTDC_LIPCR = settings->interrupt_line;
    else
        LTDC_LIPCR = LTDC_LIPCR_LIPOS_MASK << LTDC_LIPCR_LIPOS_SHIFT;

    // Load each layer's settings.
    lcd_load_layer_settings(LTDC_LAYER_1, &settings->layer1);
    lcd_load_layer_settings(LTDC_LAYER_2, &settings->layer2);

    // Load shadow registers.
    if (immediate)
        LTDC_SRCR = LTDC_SRCR_IMR;
    else
        LTDC_SRCR = LTDC_SRCR_VBR;

    // Save for next time.
    if (&current_settings != settings)
        current_settings = *settings;
}

extern void init_lcd(const lcd_config *cfg)
{
    current_config = *cfg;

    // Configure GPIO pins.
    gpio_init_pins(gpio_pins, gpio_pin_count);

    // Calculate PLL coefficients.
    uint32_t sain, sair;
    if (cfg->sair) {            // program clock by PLL constants
        sain = cfg->sain;
        sair = cfg->sair;
    } else if (cfg->dotclock > 10000) { // program by dot clock rate
        assert(0 && "write me!");
    } else {
        uint32_t clocks_per_row =
            cfg->h_sync + cfg->h_back_porch + cfg->width + cfg->h_front_porch;
        uint32_t rows_per_frame =
            cfg->v_sync + cfg->v_back_porch + cfg->height + cfg->v_front_porch;
        uint32_t dot_clock = cfg->refresh * rows_per_frame * clocks_per_row;
        (void)rows_per_frame;
        (void)dot_clock;
        assert(0 && "write me!");
    }

    // Load PLL.
    uint32_t saiq = (RCC_PLLSAICFGR >> RCC_PLLSAICFGR_PLLSAIQ_SHIFT) &
        RCC_PLLSAICFGR_PLLSAIQ_MASK;
    RCC_PLLSAICFGR = (sain << RCC_PLLSAICFGR_PLLSAIN_SHIFT |
                      saiq << RCC_PLLSAICFGR_PLLSAIQ_SHIFT |
                      sair << RCC_PLLSAICFGR_PLLSAIR_SHIFT);
    RCC_DCKCFGR |= RCC_DCKCFGR_PLLSAIDIVR_DIVR_2;
    RCC_CR |= RCC_CR_PLLSAION;
    while ((RCC_CR & RCC_CR_PLLSAIRDY) == 0)
        continue;
    RCC_APB2ENR |= RCC_APB2ENR_LTDCEN;

    // Busy-wait until the CPU has been up for 16 msec.
    // The KD50G21 datasheet says that power needs to be stable for
    // 16 msec before the video signal starts.
    while (system_millis < 16)
        continue;
    
    // Configure the Synchronous timings: VSYNC, HSYNC,
    // Vertical and Horizontal back porch, active data area, and
    // the front porch timings.
    //
    uint32_t hsw    = cfg->h_sync - 1;
    uint32_t vsh    = cfg->v_sync - 1;
    uint32_t ahbp   = hsw  + cfg->h_back_porch;
    uint32_t avbp   = vsh  + cfg->v_back_porch;
    uint32_t aaw    = ahbp + cfg->width;
    uint32_t aah    = avbp + cfg->height;
    uint32_t totalw = aaw  + cfg->h_front_porch;
    uint32_t totalh = aah  + cfg->v_front_porch;
    LTDC_SSCR = hsw  << LTDC_SSCR_HSW_SHIFT  | vsh  << LTDC_SSCR_VSH_SHIFT;
    LTDC_BPCR = ahbp << LTDC_BPCR_AHBP_SHIFT | avbp << LTDC_BPCR_AVBP_SHIFT;
    LTDC_AWCR = aaw  << LTDC_AWCR_AAW_SHIFT  | aah  << LTDC_AWCR_AAH_SHIFT;
    LTDC_TWCR =
            totalw << LTDC_TWCR_TOTALW_SHIFT | totalh << LTDC_TWCR_TOTALH_SHIFT;

    // Configure the synchronous signals and clock polarity.
    // XXX should get this from config.
    uint32_t den = cfg->use_dither ? LTDC_GCR_DITHER_ENABLE : 0;
    LTDC_GCR |= LTDC_GCR_PCPOL_ACTIVE_LOW | den;

    LTDC_BCCR = 0x00000000;
    // Configure interrupts.
    // LTDC_IER = LTDC_IER_RRIE | LTDC_IER_TERRIE | LTDC_IER_FUIE | LTDC_IER_LIE;
    LTDC_IER = LTDC_IER_RRIE;
    nvic_enable_irq(NVIC_LCD_TFT_IRQ);

    // lcd_load_settings(settings, true);
    lcd_load_settings(&current_settings, false);

    /* Enable the LCD-TFT controller. */
    LTDC_GCR |= LTDC_GCR_LTDC_ENABLE;
}

void lcd_tft_isr(void)
{
    uint32_t isr = LTDC_ISR;
    LTDC_ICR = (LTDC_ICR_CRRIF   |
                LTDC_ICR_CTERRIF |
                LTDC_ICR_CFUIF   |
                LTDC_ICR_CLIF);
    assert(!(isr & (LTDC_ISR_TERRIF | LTDC_ISR_FUIF)));
    if ((isr & LTDC_ISR_LIF) && line_callback) {
        lcd_settings *new_settings = (*line_callback)(&current_settings);
        if (new_settings) {
            lcd_load_settings(new_settings, true);
            LTDC_ICR = LTDC_ICR_CRRIF;
        }
    }        
    if ((isr & LTDC_ISR_RRIF) && line_callback) {
        lcd_settings *new_settings = (*frame_callback)(&current_settings);
        if (new_settings)
            lcd_load_settings(new_settings, false);
    }        
    LTDC_SRCR = LTDC_SRCR_VBR;
}
