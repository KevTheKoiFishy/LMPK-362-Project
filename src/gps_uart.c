#include <string.h>
#include <stdio.h> 
#include <pico/stdlib.h>

#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "const.h"
#include "gps_uart.h"
#include "gps_time.h"

#define     GPS_UART_BUFSIZE          256
char        gps_serbuf[GPS_UART_BUFSIZE];
int         gps_seridx              =  0;

uart_hw_t * gps_uart;

// Initialize SPI1 on GPS pins.
void init_gps_uart() {
    gps_uart = uart_get_hw(GPS_UART_PORT);

    gpio_set_function(GPS_PIN_RX, GPIO_FUNC_UART);
    gpio_set_function(GPS_PIN_TX, GPIO_FUNC_UART);

    uart_set_translate_crlf(GPS_UART_PORT, PICO_UART_DEFAULT_CRLF);

    uart_set_format(GPS_UART_PORT, 8, 1, UART_PARITY_NONE);

    uart_get_hw(GPS_UART_PORT) -> cr    = UART_UARTCR_UARTEN_BITS | UART_UARTCR_TXE_BITS | UART_UARTCR_RXE_BITS;
    uart_get_hw(GPS_UART_PORT) -> dmacr = UART_UARTDMACR_TXDMAE_BITS | UART_UARTDMACR_RXDMAE_BITS;

    uart_init(GPS_UART_PORT, GPS_UART_BAUD);

    uart_set_fifo_enabled(GPS_UART_PORT, false);
    uart_set_irqs_enabled(GPS_UART_PORT, true, false);
    irq_set_exclusive_handler(GPS_UART_IRQ, gps_uart_rx_handler);
    irq_set_priority(GPS_UART_IRQ, GPS_UART_INT_PRI);
    irq_set_enabled(GPS_UART_IRQ, true);
}

// Read from GPS and pipe to terminal
void gps_uart_rx_handler() {

    // Reading from register will also clear the intr
    uint16_t rcv = gps_uart -> dr & 0x7FF;
    uint8_t  err = rcv >> 8; if (err) { return; }
    uint8_t  ch  = rcv & 0xFF;

    // Prevent buffer overflow
    if (gps_seridx >= GPS_UART_BUFSIZE) { return; }

    // Track newline
    if (ch == '\n') {
        gps_parseline(gps_serbuf, gps_seridx);
        gps_seridx = 0;
    }

    // Handle backspace
    if (ch == '\b') {
        if (gps_seridx == 0) { return; }
        
        #ifdef GPS_UART_ECHO
        UART_WAIT(); uart0_hw -> dr = '\b';
        UART_WAIT(); uart0_hw -> dr = ' '; 
        UART_WAIT(); uart0_hw -> dr = '\b';
        #endif

        gps_serbuf[--gps_seridx] = (char)'\0';
    }
    // Handle echo
    else {
        #ifdef GPS_UART_ECHO
        UART_WAIT(); uart0_hw -> dr = ch;
        #endif

        gps_serbuf[gps_seridx++] = ch;
    }
}

// Write to GPS
void gps_uart_writebuf(char *buffer, uint16_t length) {
    for (uint16_t i = 0; i < length; ++i) {
        GPS_UART_WAIT(); gps_uart -> dr = buffer[i];
    }
}