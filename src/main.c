#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "sd.h"
#include "const.h"
#include "sd_uart.h"
#include <stdio.h>
#include <string.h>

int main() {
    // Initialize the standard input/output library
    init_uart();
    init_uart_irq();
    
    init_sdcard_io();
    
    // SD card functions will initialize everything.
    sd_command_shell();

    for(;;);
}