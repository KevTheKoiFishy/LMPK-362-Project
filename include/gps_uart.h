#include <stdio.h> 

#define GPS_UART_BAUD 9600

void    init_gps_uart();
void    gps_uart_rx_handler();
int     gps_uart_readline(__unused int handle, char *buffer, int length);
int     gps_uart_writebuf(__unused int handle, char *buffer, int length);