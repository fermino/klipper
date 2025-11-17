#pragma once

#include "hal/adc_types.h"

struct gpio_adc {
    adc_unit_t adc_unit;
    adc_channel_t adc_channel;
};

struct gpio_adc gpio_adc_setup(uint8_t pin);
uint32_t gpio_adc_sample(struct gpio_adc gpio);
uint16_t gpio_adc_read(struct gpio_adc gpio);
void gpio_adc_cancel_sample(struct gpio_adc gpio);
