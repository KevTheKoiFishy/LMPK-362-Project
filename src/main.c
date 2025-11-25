#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "const.h"
#include "sd.h"
#include "sd_sdcard.h"
#include "sd_uart.h"
#include "sd_audio.h"
#include "gps_uart.h"
#include "gps_time.h"

// #define TEST_SD_CMD
// #define TEST_SD_READ_RATE
// #define TEST_SD_AUDIO_PLAYBACK
# define TEST_GPS_TIME

void core_1_main() {
    core1_loop:
        core1_maintain_audio_buff_routine();
    goto core1_loop;
}

int main() {

    // SETUP AND INTERRUPTS HERE
    /**
     * SD Card Initialization
     * SD Card-Audio Initialization
    */

    /////////////////
    // MULTICORE!  //
    /////////////////
    stdio_init_all(); // Enable multicore
    multicore_launch_core1(&core_1_main);

    /////////////////
    // SD CARD     //
    /////////////////
    
    // Initialize the standard input/output library
    init_uart();
    init_uart_irq();
    init_sdcard_io();
    
    #ifdef TEST_SD_CMD
        sd_command_shell(); // uncomment for debug
    #endif

    /////////////////
    // SD-AUDIO    //
    /////////////////
    mount(0, NULL);
    
    #ifdef TEST_SD_READ_RATE
        open_sd_audio_file(DEFAULT_AUDIO_PATH);
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
        open_sd_audio_file(DEFAULT_AUDIO_PATH);
        audio_file_lseek(0x5500);
        configure_audio_play();
        start_audio_playback();
    #endif

    /////////////////
    // GPS-UART    //
    /////////////////
    init_gps_uart();

    core0_loop:
        #ifdef TEST_GPS_TIME
            sleep_ms(1000);
            printf("\nGPS Time: "); printf(gps_get_timestr()); printf("\n\n");
        #endif
    goto core0_loop;
    
}