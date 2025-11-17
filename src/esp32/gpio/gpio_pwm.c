#include "gpio/gpio_pwm.h"

#include "autoconf.h"
#include "command.h"
#include "driver/ledc.h"

/**
 * The LEDC peripheral has 4 "low speed" timers that are then muxed to 8
 * channels. This function assigns a timer to the caller to avoid having
 * two timers with equal configuration, then sets it up. Keep in mind that
 * in this context, "low speed" does not indicate a frequency limit but
 * rather the steps required to refresh the configuration after a change.
 *
 * https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/ledc.html
 */
static ledc_timer_t setup_timer(uint32_t frequency)
{
    static uint32_t assigned_freqs[LEDC_TIMER_MAX];

    int16_t timer_found = -1;
    for (uint8_t i = 0; i < LEDC_TIMER_MAX; i++) {
        // If the requested frequency is already on a timer
        if (assigned_freqs[i] == frequency) {
            timer_found = i;
            break;
        }

        // If there's an empty timer to use
        if (assigned_freqs[i] == 0) {
            assigned_freqs[i] = frequency;
            timer_found = i;
            break;
        }
    }

    if (timer_found == -1) {
        shutdown("Max amount of PWM timers assigned. See if you can use the same frequency.");
    }

    ESP_ERROR_CHECK(ledc_timer_config(&(ledc_timer_config_t) {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = timer_found,
        .freq_hz = frequency,
        .clk_cfg = LEDC_AUTO_CLK,
    }));

    return timer_found;
}

/**
 * The LEDC peripheral has 8 channels. This keeps track of them and assigns
 * them on a first come, first served basis.
 */
static ledc_channel_t setup_channel(ledc_timer_t timer, uint8_t pin, uint8_t duty)
{
    static uint8_t assigned_channels = 0;

    if (assigned_channels >= LEDC_CHANNEL_MAX) {
        shutdown("Max amount of PWM channels assigned.");
    }

    ESP_ERROR_CHECK(ledc_channel_config(&(ledc_channel_config_t) {
        .gpio_num = pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = assigned_channels,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = timer,
        .duty = duty,
        .hpoint = 0,
    }));

    return assigned_channels++;
}

/**
 * Set up a PWM pin. The max number of channels (pins) is 8, the max number of
 * timers (different frequencies) is 4.
 * @todo see https://github.com/fermino/klipper-esp32-port/issues/15, it should be fixed here too
 */
struct gpio_pwm gpio_pwm_setup(uint8_t pin, uint32_t cycle_time, uint8_t val)
{
    if (pin >= GPIO_NUM_MAX) {
        shutdown("PWM pin outside of range");
    }

    ledc_timer_t timer = setup_timer(CONFIG_CLOCK_FREQ / cycle_time);
    ledc_channel_t channel = setup_channel(timer, pin, val);

    return (struct gpio_pwm) {
        .ledc_channel = channel
    };
}
