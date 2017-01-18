#include "lcd-pwm.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include "gpio.h"

// PWM brightness pin
//   Pin PF6
//   Timer TIM10, channel 1
//   Alt function AF3

// Display On/Off pin
//   Pin PE4


#define PWM_PORT  GPIOF
#define PWM_PIN   GPIO6
#define DISP_PORT GPIOE
#define DISP_PIN  GPIO4

static const gpio_pin lcd_pwm_pins[] = {
    {
        .gp_port  = PWM_PORT,
        .gp_pin   = PWM_PIN,
        .gp_mode  = GPIO_MODE_AF,
        .gp_af    = GPIO_AF3,
    },
    {
        .gp_port  = DISP_PORT,
        .gp_pin   = DISP_PIN,
        .gp_mode  = GPIO_MODE_OUTPUT,
        .gp_level = 0,
    },
};
static const size_t lcd_pwm_pin_count = (&lcd_pwm_pins)[1] - lcd_pwm_pins;

void lcd_pwm_setup(void)
{
    // enable clock
    rcc_periph_clock_enable(RCC_TIM10);

    // configure timer
    timer_reset(TIM10);
    timer_set_oc_mode(TIM10, TIM_OC1, TIM_OCM_PWM1);
    timer_enable_oc_output(TIM10, TIM_OC1);
    timer_set_oc_value(TIM10, TIM_OC1, 0); // start out dark.
    // FAN5333BSX datasheet says PWM frequency not to exceed 1 KHz.
    // 168 MHz / 65536 / 3 ~= 854 Hz.
    timer_set_period(TIM10, 65536 - 1);
    timer_set_prescaler(TIM10, 3 - 1);
    timer_enable_counter(TIM10);

    // init GPIOs
    gpio_init_pins(lcd_pwm_pins, lcd_pwm_pin_count);
}

void lcd_pwm_set_brightness(uint16_t brightness)
{
    timer_set_oc_value(TIM10, TIM_OC1, brightness);
    if (brightness)
        gpio_set(DISP_PORT, DISP_PIN);
    else
        gpio_clear(DISP_PORT, DISP_PIN);
}

void lcd_fade(uint16_t level0,
              uint16_t level1,
              uint32_t delay_msec,
              uint32_t duration_msec)
{
    // config params;
    uint32_t now = system_millis;
    fader.l0 = level0;
    fader.l1 = level1;
    fader.t0 = now + delay_msec;
    fader.t1 = now + delay_msec + duration_msec;

    // enable timer interrupt
    nvic_enable_irq(NVIC_TIM1_UP_TIM10_IRQ);
    timer_enable_irq(TIM10, TIM_DIER_UIE);
}

void tim1_up_tim10_isr(void)
{
    // get the time.
    uint32_t now = system_millis;
    int32_t dt = now - fader.t0;
    uint32_t dur = fader.t1 - fader.t0;
    if (dt > (int32_t)dur)
        dt = dur;                // if dur == 0, use final value
    else if (dt < 0)
        dt = 0;

    // lerp the brightness
    uint32_t l0 = fader.l0;
    uint32_t l1 = fader.l1;
    uint32_t base = l0;
    int32_t delta = (int32_t)l1 - (int32_t)l0;
    uint32_t bright = base + (delta * dt) / dur;

    // square for smoother ramping
    bright = (bright * bright) >> 16;
    lcd_pwm_set_brightness(bright);

    // stop interrupt if done.
    if (dt == (int32_t)dur)
        timer_disable_irq(TIM10, TIM_DIER_UIE);
}
