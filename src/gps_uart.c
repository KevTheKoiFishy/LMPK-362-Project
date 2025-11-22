#include <string.h>
#include <stdio.h> 
#include <pico/stdlib.h>

#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "const.h"
#include "gps_uart.h"

#define     GPS_UART_BUFSIZE          256
char        gps_serbuf[GPS_UART_BUFSIZE];
int         gps_seridx              =  0;
int         gps_newline_seen        =  0;

// Initialize SPI1 on GPS pins.
void init_gps_uart() {
    gpio_set_function(GPS_PIN_RX, UART_FUNCSEL_NUM(GPS_UART_PORT, GPS_PIN_RX));
    gpio_set_function(GPS_PIN_TX, UART_FUNCSEL_NUM(GPS_UART_PORT, GPS_PIN_TX));

    uart_set_translate_crlf(GPS_UART_PORT, PICO_UART_DEFAULT_CRLF);

    uart_set_format(GPS_UART_PORT, 8, 1, UART_PARITY_NONE);

    uart_get_hw(GPS_UART_PORT) -> cr    = UART_UARTCR_UARTEN_BITS | UART_UARTCR_TXE_BITS | UART_UARTCR_RXE_BITS;
    uart_get_hw(GPS_UART_PORT) -> dmacr = UART_UARTDMACR_TXDMAE_BITS | UART_UARTDMACR_RXDMAE_BITS;

    uart_init(GPS_UART_PORT, GPS_UART_BAUD);

    uart_set_fifo_enabled(GPS_UART_PORT, false);
    uart_set_irqs_enabled(GPS_UART_PORT, true, false);
    irq_set_exclusive_handler(GPS_UART_IRQ, gps_uart_rx_handler);
    irq_set_enabled(GPS_UART_IRQ, true);
}

// Read from GPS and pipe to terminal
void gps_uart_rx_handler() {
    // Prevent buffer overflow
    if (gps_seridx >= GPS_UART_BUFSIZE) { return; }

    uart_hw_t * gps_uart = uart_get_hw(GPS_UART_PORT);

    // Trigger only if received data
    if (!(gps_uart -> mis & UART_UARTMIS_RXMIS_BITS)) { return; }

    // Ack
    gps_uart -> icr = UART_UARTICR_RXIC_BITS;

    // Reading from register will also clear the intr
    uint16_t rcv = gps_uart -> dr & 0x7FF;
    uint8_t  err = rcv >> 8; if (err) { return; }
    uint8_t  ch  = rcv & 0xFF;

    // Track newline
    gps_newline_seen = ch == '\n';

    // Handle backspace
    if (ch == '\b') {
        if (gps_seridx == 0) { return; }
        
        UART_WAIT(); uart0_hw -> dr = '\b';
        UART_WAIT(); uart0_hw -> dr = ' '; 
        UART_WAIT(); uart0_hw -> dr = '\b';

        gps_serbuf[--gps_seridx] = (char)'\0';
    }
    // Handle echo
    else {
        uart0_hw -> dr   = ch;
        gps_serbuf[gps_seridx++] = ch;
    }
}

// Read from GPS
int gps_uart_readline(__unused int handle, char *buffer, int length) {
    // Wait for delimiter
    while (!gps_newline_seen) { sleep_ms(5); }
    gps_newline_seen = false;

    buffer[gps_seridx] = '\0';
    // Strcpy and reset serbuf
    do {
        --gps_seridx;
        buffer[gps_seridx] = gps_serbuf[gps_seridx];
        gps_serbuf[gps_seridx] = 0;
    }
    while (gps_seridx);

    return length;
}

// Write to GPS
int gps_uart_writebuf(__unused int handle, char *buffer, int length) {
    uint8_t i;
    for (i = 0; i < length; ++i) {
        uart_putc(GPS_UART_PORT, buffer[i]);
    }
    return i;
}