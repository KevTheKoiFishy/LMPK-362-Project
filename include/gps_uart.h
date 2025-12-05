#include <stdio.h> 

#define GPS_UART_BAUD 115200
// #define GPS_UART_ECHO

void    init_gps_uart();
void    disable_gps_uart();
void    gps_uart_rx_handler();
void    gps_uart_writebuf(char *buffer, uint16_t length);