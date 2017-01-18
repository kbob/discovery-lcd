#ifndef LCD_PWM_included
#define LCD_PWM_included

#include <stdint.h>

extern void lcd_pwm_setup(void);

extern void lcd_pwm_set_brightness(uint16_t brightness);

extern void lcd_fade(uint16_t level0,
                     uint16_t level1,
                     uint32_t delay_msec,
                     uint32_t duration_msec);

#endif /* !LCD_PWM_included */
