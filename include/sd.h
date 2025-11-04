#pragma once
#include <stdint.h>

uint32_t    init_sd_spi();
void        deinit_sd_spi();
uint8_t     mount_sd_card();
void        unmount_sd_card();