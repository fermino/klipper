#pragma once

#include <stdint.h>

// ReSharper disable CppUnusedIncludeDirective
#include "gpio/gpio.h"
#include "gpio/gpio_out.h"
#include "gpio/gpio_in.h"
#include "gpio/gpio_pwm.h"
// ReSharper restore CppUnusedIncludeDirective

/**
 * @todo OTHERSADASDASDS
 */

struct spi_config {
    uint8_t spcr, spsr;
};
struct spi_config spi_setup(uint32_t bus, uint8_t mode, uint32_t rate);
void spi_prepare(struct spi_config config);
void spi_transfer(struct spi_config config, uint8_t receive_data
                  , uint8_t len, uint8_t *data);

struct i2c_config {
    uint8_t addr;
};

struct i2c_config i2c_setup(uint32_t bus, uint32_t rate, uint8_t addr);
int i2c_write(struct i2c_config config, uint8_t write_len, uint8_t *write);
int i2c_read(struct i2c_config config, uint8_t reg_len, uint8_t *reg
              , uint8_t read_len, uint8_t *read);