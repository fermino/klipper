#include "gpio/gpio_adc.h"
#include "command.h"
#include "soc/adc_periph.h"

/**
 * ESP32 and its variants may have one or more ADC units, and the pins are
 * hardwired to the ADC mux, so no reassigning.
 */
struct gpio_adc gpio_adc_setup(uint8_t pin)
{
    for (uint8_t i = 0; i < SOC_ADC_PERIPH_NUM; i++) {
        for (uint8_t j = 0; j < SOC_ADC_MAX_CHANNEL_NUM; j++) {
            if (adc_channel_io_map[i][j] == pin) {
                return (struct gpio_adc) {
                    .adc_unit = i,
                    .adc_channel = j,
                };
            }
        }
    }

    shutdown("ADC pin is not a valid ADC channel.");
}

uint32_t gpio_adc_sample(struct gpio_adc gpio)
{
}

uint16_t gpio_adc_read(struct gpio_adc gpio)
{
}

void gpio_adc_cancel_sample(struct gpio_adc gpio)
{
}
