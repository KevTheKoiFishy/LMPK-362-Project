#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "hardware/spi.h"

#include "sd.h"
#include "const.h"

uint32_t init_spi_sdcard_with_baud(uint32_t baud) {
    // Initialize SPI
    baud = spi_init(SD_SPI_PORT, baud);
    // Set SPI format to 8 bits, mode 0
    spi_set_format(SD_SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    // Configure GPIO pins for SPI
    gpio_set_function(SD_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(SD_PIN_CS, GPIO_FUNC_SIO);
    gpio_set_function(SD_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SD_PIN_MOSI, GPIO_FUNC_SPI);
    // Set CS pin high (inactive)
    gpio_set_dir(SD_PIN_CS, GPIO_OUT);
    gpio_put(SD_PIN_CS, 1);

    //printf("Initialized SD SPI with baud rate: %ld\n", baud);
    return baud;
}

void init_spi_sdcard() {
    init_spi_sdcard_with_baud(SD_SPI_BAUD_SLOW);
}

void disable_sdcard() {
    // Send filler bytes
    uint16_t filler[1] = {0xFFFF};
    spi_write16_blocking(SD_SPI_PORT, filler, 1);

    // force mosi high
    gpio_set_function(SD_PIN_MOSI, GPIO_FUNC_SIO);
    gpio_put(SD_PIN_MOSI, 1);
}

void enable_sdcard() {
    gpio_set_function(SD_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_put(SD_PIN_CS, 0);
}

void sdcard_io_high_speed() {
    init_spi_sdcard_with_baud(SD_SPI_BAUD);
}

void init_sdcard_io() {
    init_spi_sdcard();
    disable_sdcard();
}