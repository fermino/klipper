#pragma once

#include "hal/ledc_types.h"
#include "hal/ledc_ll.h"

struct gpio_pwm {
    ledc_channel_t ledc_channel;
};

struct gpio_pwm gpio_pwm_setup(uint8_t pin, uint32_t cycle_time, uint8_t val);

static inline void __attribute__((always_inline)) gpio_pwm_write(struct gpio_pwm gpio, uint8_t val)
{
    // @todo critical section
    // @todo some of this could be removed
    ledc_ll_set_duty_int_part(LEDC_LL_GET_HW(), LEDC_LOW_SPEED_MODE, gpio.ledc_channel, val);
    ledc_ll_set_sig_out_en(LEDC_LL_GET_HW(), LEDC_LOW_SPEED_MODE, gpio.ledc_channel, true);
    ledc_ll_set_duty_start(LEDC_LL_GET_HW(), LEDC_LOW_SPEED_MODE, gpio.ledc_channel);
    ledc_ll_ls_channel_update(LEDC_LL_GET_HW(), LEDC_LOW_SPEED_MODE, gpio.ledc_channel);
}
