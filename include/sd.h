#define SD_SPI_BAUD         20000000
#define SD_SPI_BAUD_SLOW    100000
#define SD_SPI_USE_HIGH_SPEED

uint32_t init_spi_sdcard_with_baud(uint32_t baud);
void init_spi_sdcard();
void disable_sdcard();
void enable_sdcard();
void sdcard_io_high_speed();
void init_sdcard_io();