#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "const.h"
#include "sd.h"
#include "sd_sdcard.h"
#include "sd_uart.h"
#include "sd_audio.h"

// #define  TEST_SD_READ_RATE
#define TEST_SD_AUDIO_PLAYBACK

int main() {

    // SETUP AND INTERRUPTS HERE
    /**
     * SD Card Initialization
     * SD Card-Audio Initialization
    */

    /////////////////
    // SD CARD     //
    /////////////////
    
    // Initialize the standard input/output library
    init_uart();
    init_uart_irq();
    init_sdcard_io();
    
    // SD card functions will initialize everything.
    sd_command_shell(); // uncomment for debug

    /////////////////
    // SD-AUDIO    //
    /////////////////
    stdio_init_all(); // Enable multicore
    mount(0, NULL);
    open_sd_audio_file(DEFAULT_AUDIO_PATH);

#ifdef TEST_SD_READ_RATE
    audio_file_result buff_load_result = 0;
    uint32_t pos_0 = get_data_offset();
    uint32_t tick  = time_us_32();

    uint16_t i = 1000;
    while(!buff_load_result && --i){
        buff_load_result = fill_audio_buffer();
    }

    uint32_t tock     = time_us_32();
    uint32_t pos_f    = get_data_offset();
    float    byterate = 1000000.0 * (pos_f - pos_0) / (tock - tick);
    printf("Average Read Byte Rate: %lu bytes over %lu us = %.2f bytes/sec", pos_f - pos_0, tock - tick, byterate);
#endif

#ifdef TEST_SD_AUDIO_PLAYBACK
    audio_file_lseek(0x5500);
    fill_audio_buffer();
    configure_audio_play();
    // multicore_launch_core1(&configure_audio_play);
    start_audio_playback();
#endif

    // LONG, BLOCKING PROCESSES IN LOOP
    /**
     * Display Rendering and Refersh
     */

    while (1){
        
    }
    
}