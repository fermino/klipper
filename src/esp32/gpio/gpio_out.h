#pragma once

#include "gpio/gpio.h"
#include "hal/gpio_ll.h"
#include "gpio/shift_register.h"

struct gpio_out gpio_out_setup(gpio_num_t pin, uint_fast8_t val);
void gpio_out_reset(struct gpio_out gpio, uint_fast8_t val);

static inline void __attribute__((always_inline)) gpio_out_write(struct gpio_out gpio, uint_fast8_t val)
{
    if (gpio_is_sr(gpio)) {
        gpio_sr_write(gpio, val);
    } else {
        gpio_ll_set_level(GPIO_LL_GET_HW(GPIO_PORT_0), gpio.pin, val);
    }
}

static inline void __attribute__((always_inline)) gpio_out_toggle_noirq(struct gpio_out gpio)
{
    if (gpio_is_sr(gpio)) {
        gpio_sr_write(gpio, !gpio_sr_read(gpio));
    } else {
        gpio_out_write(gpio, !gpio_ll_get_level(GPIO_LL_GET_HW(GPIO_PORT_0), gpio.pin));
    }
}

static inline void __attribute__((always_inline)) gpio_out_toggle(struct gpio_out gpio)
{
    gpio_out_toggle_noirq(gpio);
}
