#pragma once

#include "gpio/gpio.h"
#include "autoconf.h"
#include <stdbool.h>
#include <stdint.h>

void gpio_sr_shift_out();

// @todo clause

/**
 * This function should be optimized out if the feature is
 * disabled (CONFIG_HAVE_GPIO_SR == false)
 */
static inline bool __attribute__((always_inline)) gpio_is_sr(struct gpio_out gpio)
{
    return CONFIG_HAVE_GPIO_SR && (gpio.pin & 0b10000000);
}

static inline uint8_t __attribute__((always_inline)) gpio_sr_bit_index(struct gpio_out gpio)
{
    return gpio.pin & 0b111;
}

static inline uint8_t __attribute__((always_inline)) gpio_sr_byte_index(struct gpio_out gpio)
{
    return (gpio.pin & 0b01111000) >> 3;
}

static inline void __attribute__((always_inline)) gpio_sr_write(struct gpio_out gpio, bool val)
{
    extern volatile uint8_t sr_data[CONFIG_SR_BYTE_NO];

    if (val) {
        sr_data[gpio_sr_byte_index(gpio)] |= 1 << gpio_sr_bit_index(gpio);
    } else {
        sr_data[gpio_sr_byte_index(gpio)] &= ~(1 << gpio_sr_bit_index(gpio));
    }

    gpio_sr_shift_out();
}

static inline bool __attribute__((always_inline)) gpio_sr_read(struct gpio_out gpio)
{
    extern volatile uint8_t sr_data[CONFIG_SR_BYTE_NO];

    return (bool) sr_data[gpio_sr_byte_index(gpio)] & (1 << gpio_sr_bit_index(gpio));
}
