#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"

#include <hardware/spi.h>
#include <tf_card.h>

#include "const.h"
#include "sd.h"

// Circular buffer to hold audio samples read from SD card
#define            AUDIO_BUFFER_LEN 4096
const     int16_t  AUDIO_BUFFER [AUDIO_BUFFER_LEN] = {};
volatile uint16_t  I_buffer_write  = 0;
volatile uint16_t  I_buffer_read   = 0;

// Threshold to trigger start loading and stop loading more audio data
const    uint16_t  LOAD_WHEN       = AUDIO_BUFFER_LEN >> 2;

uint16_t get_buff_avail();

FILE * audio_file = NULL;