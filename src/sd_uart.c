#include <string.h>
#include <stdio.h> 
 
#include "hardware/uart.h" 
#include "hardware/gpio.h" 
#include "pico/stdlib.h"
#include "sd_sdcard.h"
#include "sd_uart.h"
#include "const.h"

#define     UART_BUFSIZE          256
char        serbuf[UART_BUFSIZE]    ;
int         seridx              =  0;
int         newline_seen        =  0;

/*******************************************************************/

void init_uart() {
    gpio_set_function(0, UART_FUNCSEL_NUM(uart0, 0));
    gpio_set_function(1, UART_FUNCSEL_NUM(uart0, 1));
    
    // uart_reset(uart0);
    // uart_unreset(uart0);

    uart_set_translate_crlf(uart0, PICO_UART_DEFAULT_CRLF);

    // uart_set_baudrate(uart0, 115200);
    uart_set_format(uart0, 8, 1, UART_PARITY_NONE);

    uart_get_hw(uart0) -> cr    = UART_UARTCR_UARTEN_BITS | UART_UARTCR_TXE_BITS | UART_UARTCR_RXE_BITS;
    uart_get_hw(uart0) -> dmacr = UART_UARTDMACR_TXDMAE_BITS | UART_UARTDMACR_RXDMAE_BITS;

    uart_init(uart0, 115200);
}

void init_uart_irq() {
    uart_set_fifo_enabled(uart0, false);
    uart_set_irqs_enabled(uart0, true, false);
    irq_set_exclusive_handler(UART0_IRQ, uart_rx_handler);
    irq_set_enabled(UART0_IRQ, true);
}

void uart_rx_handler() {
    // Prevent buffer overflow
    if (seridx >= UART_BUFSIZE) { return; }

    // Trigger only if received data
    if (!(uart0_hw -> mis & UART_UARTMIS_RXMIS_BITS)) { return; }

    // Ack
    uart0_hw -> icr = UART_UARTICR_RXIC_BITS;

    // Reading from register will also clear the intr
    uint16_t rcv = uart0_hw -> dr & 0x7FF;
    uint8_t  err = rcv >> 8; if (err) { return; }
    uint8_t  ch  = rcv & 0xFF;

    // Track newline
    newline_seen = ch == '\n';

    // Handle backspace
    if (ch == '\b') {
        if (seridx == 0) { return; }
        
        UART_WAIT(); uart0_hw -> dr   = '\b';
        UART_WAIT(); uart0_hw -> dr   = ' '; 
        UART_WAIT(); uart0_hw -> dr   = '\b';

        serbuf[--seridx] = (char)'\0';
    }
    // Handle echo
    else {
        uart0_hw -> dr   = ch;
        serbuf[seridx++] = ch;
    }
}

int _read(__unused int handle, char *buffer, int length) {
    // Wait for delimiter
    while (!newline_seen) { sleep_ms(5); }
    newline_seen = false;

    buffer[seridx] = '\0';
    // Strcpy and reset serbuf
    do {
        --seridx;
        buffer[seridx] = serbuf[seridx];
        serbuf[seridx] = 0;
    }
    while (seridx);

    return length;
}

int _write(__unused int handle, char *buffer, int length) {
    uint8_t i;
    for (i = 0; i < length; ++i) {
        uart_putc(uart0, buffer[i]);
    }
    return i;
}

/***********************************1********************************/

struct commands_t {
    const char *cmd;
    void      (*fn)(int argc, char *argv[]);
};

struct commands_t cmds[] = {
        { "append", append },
        { "cat", cat },
        { "cd", cd },
        { "date", date },
        { "input", input },
        { "ls", ls },
        { "mkdir", mkdir },
        { "mount", mount },
        { "pwd", pwd },
        { "rm", rm },
        { "restart", restart }
};

// This function inserts a string into the input buffer and echoes it to the UART
// but whatever is "typed" by this function can be edited by the user.
void insert_echo_string(const char* str) {
    // Print the string and copy it into serbuf, allowing editing
    seridx = 0;
    newline_seen = 0;
    memset(serbuf, 0, UART_BUFSIZE);

    // Print and fill serbuf with the initial string
    for (int i = 0; str[i] != '\0' && seridx < UART_BUFSIZE - 1; i++) {
        char c = str[i];
        uart_write_blocking(uart0, (uint8_t*)&c, 1);
        serbuf[seridx++] = c;
    }
}

void parse_command(const char* input) {
    char *token = strtok(input, " ");
    int argc = 0;
    char *argv[10];
    while (token != NULL && argc < 10) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    
    int i = 0;
    for(; i<sizeof cmds/sizeof cmds[0]; i++) {
        if (strcmp(cmds[i].cmd, argv[0]) == 0) {
            cmds[i].fn(argc, argv);
            break;
        }
    }
    if (i == (sizeof cmds/sizeof cmds[0])) {
        printf("Unknown command: %s\n", argv[0]);
    }
}

void sd_command_shell() {
    char input[100];
    memset(input, 0, sizeof(input));

    // Disable buffering for stdout
    setbuf(stdout, NULL);

    printf("\nEnter current ");
    insert_echo_string("date 20250701120000");
    fgets(input, 99, stdin);
    input[strcspn(input, "\r")] = 0; // Remove CR character
    input[strcspn(input, "\n")] = 0; // Remove newline character
    parse_command(input);
    
    printf("SD Card Command Shell");
    printf("\r\nType 'mount' to mount the SD card.\n");
    while (1) {
        printf("\r\n> ");
        fgets(input, sizeof(input), stdin);
        fflush(stdin);
        input[strcspn(input, "\r")] = 0; // Remove CR character
        input[strcspn(input, "\n")] = 0; // Remove newline character
        
        parse_command(input);
    }
}