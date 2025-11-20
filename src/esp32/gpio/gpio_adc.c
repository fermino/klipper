#include "gpio/gpio_adc.h"
#include "command.h"
#include "esp_clk_tree.h"
#include "esp_private/gpio.h"
#include "esp_private/regi2c_ctrl.h"
#include "esp_private/sar_periph_ctrl.h"
#include "generic/misc.h"
#include "hal/adc_ll.h"
#include "hal/adc_oneshot_hal.h"
#include "soc/adc_periph.h"


/**
 * The current implementation does not support the ADC in RISC-V cores because
 * there are differences in how it's clocked internally. If you want to port it,
 * you're welcome to step through the `peripherals/adc/oneshot_read` example
 * from ESP-IDF and modify things here accordingly. In the meantime, I figured
 * it would add a lot of complexity without much reason since I have not seen
 * a 3d printer motherboard with an ESP32 from the C/H/P series yet.
 */
#if !defined(CONFIG_IDF_TARGET_ARCH_XTENSA) || !CONFIG_IDF_TARGET_ARCH_XTENSA
#   error "Klipper's ESP32 ADC implementation does not support RISC-V cores due to clock configuration differences. You're welcome to port it :)"
#endif

#define ADC_LL_EVENT_ONESHOT_DONE_FROM_UNIT(unit) ((unit) == ADC_UNIT_1 ? ADC_LL_EVENT_ADC1_ONESHOT_DONE : ADC_LL_EVENT_ADC2_ONESHOT_DONE)

/**
 * Init the ADC units. Different ESP variants have different amounts of ADC
 * units (1 or 2). Here we start all of them so that all the channels are
 * available if needed.
 */
static adc_oneshot_hal_ctx_t adc_hal[SOC_ADC_PERIPH_NUM];
void adc_init()
{
    uint32_t clk_src_freq_hz = 0;
    esp_clk_tree_src_get_freq_hz(
        ADC_DIGI_CLK_SRC_DEFAULT,
        ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,
        &clk_src_freq_hz
    );

    for (uint8_t i = 0; i < SOC_ADC_PERIPH_NUM; i++) {
        adc_oneshot_hal_init(&adc_hal[i], &(adc_oneshot_hal_cfg_t) {
            .unit = i,
            .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
            .clk_src_freq_hz = clk_src_freq_hz,
            .work_mode = ADC_HAL_SINGLE_READ_MODE,
        });
        sar_periph_ctrl_adc_oneshot_power_acquire();
    }
}
DECL_INIT(adc_init);

/**
 * Set up an ADC pin. ESP-IDF only provides ADC->GPIO mapping tables, so we
 * have to do a reverse lookup to get the ADC unit and channel.
 *
 * Here we also define the resolution (bitwidth) of the channel readings,
 * and inform Klippy of what values to expect by defining ADC_MAX through
 * the DECL_CONSTANT macro.
 *
 * Based on the section 5.5 of the datasheet, a 12dB attenuation should give us
 * a range of around 0-3V.
 *
 * @todo: use ADC calibration
 * @todo add esp error checks
 * @todo DISABLE IRQ
 */
DECL_CONSTANT("ADC_MAX", 1023);
struct gpio_adc gpio_adc_setup(uint8_t pin)
{
    for (uint8_t i = 0; i < SOC_ADC_PERIPH_NUM; i++) {
        for (uint8_t j = 0; j < SOC_ADC_MAX_CHANNEL_NUM; j++) {
            if (adc_channel_io_map[i][j] == pin) {
                struct gpio_adc gpio = {
                    .adc_unit = i,
                    .adc_channel = j,
                };

                gpio_config_as_analog(pin);

                adc_hal[gpio.adc_unit].chan_configs[gpio.adc_channel].atten = ADC_ATTEN_DB_12;
                adc_hal[gpio.adc_unit].chan_configs[gpio.adc_channel].bitwidth = ADC_BITWIDTH_10;

                return gpio;
            }
        }
    }

    shutdown("ADC pin is not a valid ADC channel.");
}

/**
 * Start an ADC conversion. Since not all ESPs have more than one ADC unit
 * and most MCUs don't, I decided to keep things clean and support only one
 * conversion at a time. It is technically possible to have both units
 * running simultaneously, but I don't think it's worth the complexity if
 * it's not going to be used. You're welcome to improve things, though!
 *
 * @todo USAR _lock_try_acquire o un lock nuestro!
 * @todo calibration V1
 */
volatile int32_t running_conversion_pin = -1;
uint32_t gpio_adc_sample(struct gpio_adc gpio)
{
    /**
     * If there is no conversion running, start one and save the status to
     * our FSM. The usual conversion time in an ESP32 is around 13us
     * (anecdotical evidence), so 15us should be enough so that the next call
     * to this function returns 0.
     */
    if (running_conversion_pin == -1) {
        adc_oneshot_hal_setup(&adc_hal[gpio.adc_unit], gpio.adc_channel);
        adc_oneshot_ll_start(gpio.adc_unit);
        running_conversion_pin = adc_channel_io_map[gpio.adc_unit][gpio.adc_channel];
        return timer_from_us(15);
    }

    /**
     * If there's a running conversion and matches the request, check if it
     * has finished. If it has, return 0 meaning the conversion reading is
     * available. If it hasn't, return a reasonable time in which it should
     * finish.
     */
    if (running_conversion_pin == adc_channel_io_map[gpio.adc_unit][gpio.adc_channel]) {
        if (adc_oneshot_ll_get_event(ADC_LL_EVENT_ONESHOT_DONE_FROM_UNIT(gpio.adc_unit))) {
            return 0;
        }
        return timer_from_us(5);
    }

    /**
     * If no conversion matches, throw an error.
     */
    shutdown("ADC: can't run multiple conversions at once.");
}

/**
 * Read the result of an ADC conversion. This function should be called only
 * after getting a 0 from gpio_adc_sample(), and only once.
 *
 * @todo on S2/S3 do checks when SOC_ADC_ARBITER_SUPPORTED
 * @todo calibration
 */
uint16_t gpio_adc_read(struct gpio_adc gpio)
{
    if (running_conversion_pin == -1) {
        shutdown("ADC: no conversion has been started.");
    }
    if (!adc_oneshot_ll_get_event(ADC_LL_EVENT_ONESHOT_DONE_FROM_UNIT(gpio.adc_unit))) {
        shutdown("ADC: conversion has not finished yet.");
    }

    running_conversion_pin = -1;
    return adc_oneshot_ll_get_raw_result(gpio.adc_unit);
}

/**
 * Cancel a currently running conversion. Waits until the conversion finishes
 * and reads the value so that the FSM goes back to its initial state.
 */
void gpio_adc_cancel_sample(struct gpio_adc gpio)
{
    if (running_conversion_pin == -1) {
        shutdown("ADC: there is no conversion running.");
    }

    while (gpio_adc_sample(gpio) != 0) {
        // Wait until the conversion has finished
    }
    gpio_adc_read(gpio);
}
