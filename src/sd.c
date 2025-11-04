#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"

#include <hardware/spi.h>
#include <tf_card.h>

#include "const.h"
#include "sd.h"

pico_fatfs_spi_config_t sd_config = {
    SD_SPI_PORT,
    SD_SPI_BAUD,
    50 * 1000 * 1000, // 50 MHz for fast clock
    SD_PIN_MISO,
    SD_PIN_CS,
    SD_PIN_SCK,
    SD_PIN_MOSI,
    true
};

uint32_t init_sd_spi() {
    // Initialize SPI at 1 MHz
    uint32_t baud = spi_init(SD_SPI_PORT, SD_SPI_BAUD);
    // Set SPI format to 8 bits, mode 0
    spi_set_format(SD_SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    // Configure GPIO pins for SPI
    gpio_set_function(SD_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(SD_PIN_CS, GPIO_FUNC_SPI);
    gpio_set_function(SD_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SD_PIN_MOSI, GPIO_FUNC_SPI);
    // Set CS pin high (inactive)
    gpio_set_dir(SD_PIN_CS, GPIO_OUT);
    gpio_put(SD_PIN_CS, 1);

    print("Initialized SD SPI with baud rate: %u\n", baud);

    return baud;
}

void deinit_sd_spi() {
    // Deinitialize SPI
    spi_deinit(SD_SPI_PORT);

    // Optionally, reset GPIO pins to default state
    gpio_set_function(SD_PIN_MISO, GPIO_FUNC_NULL);
    gpio_set_function(SD_PIN_CS, GPIO_FUNC_NULL);
    gpio_set_function(SD_PIN_SCK, GPIO_FUNC_NULL);
    gpio_set_function(SD_PIN_MOSI, GPIO_FUNC_NULL);
}

uint8_t mount_sd_card() {
    // Mount a FAT32 filesystem on the SD card
    if (!pico_fatfs_set_config(&sd_config)) {
        printf("SD card configured for SPI mode.\n");
    } else {
        printf("SD card configured for PIO SPI mode.\n");
    }

    if (pico_fatfs_mount()) {
        printf("SD card mounted successfully.\n");
        return 0;
    } else {
        printf("Failed to mount SD card.\n");
        return 1;
    }
}

void unmount_sd_card() {
    // Unmount the FAT32 filesystem
    pico_fatfs_unmount();
    printf("SD card unmounted.\n");
}