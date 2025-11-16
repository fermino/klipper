#include "gpio/shift_register.h"
#include "autoconf.h"
#include "command.h"
#include "esp_private/spi_common_internal.h"
#include "hal/clk_tree_hal.h"
#include "hal/spi_ll.h"

#if CONFIG_HAVE_GPIO_SR

DECL_ENUMERATION_RANGE("pin", "SR0", (1u << 7), SR_BIT_NO);

volatile uint8_t sr_data[CONFIG_SR_BYTE_NO] = {0};

void sr_init()
{
    spi_dev_t* spi = SPI_LL_GET_HW(SR_SPI_HOST);

    /**
     * Enable state machine clocks and reset everything
     */
    PERIPH_RCC_ATOMIC() {
        spi_ll_enable_bus_clock(SR_SPI_HOST, true);
        spi_ll_reset_register(SR_SPI_HOST);
    }

    /**
     * Configure IO
     */
    ESP_ERROR_CHECK(spicommon_bus_initialize_io(
        SR_SPI_HOST,
        &(spi_bus_config_t) {
            .mosi_io_num = CONFIG_SR_PIN_DATA,
            .sclk_io_num = CONFIG_SR_PIN_CLK,
            .miso_io_num = -1,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
        },
        SPICOMMON_BUSFLAG_MASTER,
        NULL
    ));
    spicommon_cs_initialize(SR_SPI_HOST, CONFIG_SR_PIN_LATCH, 0, 1);

    /**
     * Set up the whoooole thing
     */
    spi_ll_enable_clock(SR_SPI_HOST, true);
    spi_ll_set_clk_source(spi, SPI_CLK_SRC_APB);
    spi_ll_master_init(spi);
    spi_ll_master_set_line_mode(spi, (spi_line_mode_t) { 1, 1, 1});
    spi_ll_master_set_clock(spi, (int)clk_hal_apb_get_freq_hz(), (CONFIG_SR_CLOCK_FREQ_MHZ * 1000 * 1000), 128);
    spi_ll_master_set_mode(spi, 0);
    spi_ll_master_select_cs(spi, 0);
    spi_ll_master_set_cs_setup(spi, 0);
    spi_ll_master_set_cs_hold(spi, 1);
    // Set LATCH polarity (SPI CS)
    spi_ll_master_set_pos_cs(spi, 0, 0);
    spi_ll_master_keep_cs(spi, 0);
    spi_ll_set_half_duplex(spi, 0);
    spi_ll_set_sio_mode(spi, 0);
    spi_ll_set_mosi_delay(spi, 0, 0);
    spi_ll_set_mosi_free_level(spi, false);
    spi_ll_set_miso_delay(spi, 2, 0);
    spi_ll_enable_miso(spi, false);
    spi_ll_set_dummy(spi, 0);
    spi_ll_set_command_bitlen(spi, 0);
    spi_ll_set_addr_bitlen(spi, 0);
    spi_ll_set_miso_bitlen(spi, 0);
    spi_ll_clear_int_stat(spi);
    // Send LSB last
    spi_ll_set_tx_lsbfirst(spi, 0);
    spi_ll_set_mosi_bitlen(spi, SR_BIT_NO);
    spi_ll_enable_mosi(spi, true);
    spi_ll_apply_config(spi);

    // Initial write to the SR to set it to a known state
    gpio_sr_shift_out();
}
DECL_INIT(sr_init);

#endif
