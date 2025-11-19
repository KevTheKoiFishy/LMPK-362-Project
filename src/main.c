#include <stdio.h>
#include "pico/stdlib.h"

#include "const.h"
#include "sd.h"
#include "sd_sdcard.h"
#include "sd_uart.h"
#include "sd_audio.h"

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
    mount(0, NULL);
    open_sd_audio_file(DEFAULT_AUDIO_PATH);

    // LONG, BLOCKING PROCESSES IN LOOP
    /**
     * Display Rendering and Refersh
     */

    while (1){
    }
}