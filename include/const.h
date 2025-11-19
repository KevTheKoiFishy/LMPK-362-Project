// Constants and macros to make our lives easier!

// In order to prevent pin and hardware peripheral conflicts
// betwee different parts of the project, state your pin choices
// and spi/pwm/adc channels here.

// For brevity, only put stuff that you're "claiming" for your functions.
// Put all other define statements in your specific header files!

#include <hardware/spi.h>

// MACROS
#define GET_CORE_NUM()      *(uint32_t *) (SIO_BASE + SIO_CPUID_OFFSET)
#define ADC_WAIT()          while ( !(adc_hw -> cs & ADC_CS_READY_BITS) )
#define UART_WAIT()         while (!uart_is_writable(uart0)) { tight_loop_contents(); }


// ONBOARD
#define ONBOARD_LEDS        {22, 23, 24, 25}
#define ONBOARD_PUSHBUTTONS {21, 26}

// KEYPAD
#define NUM_PAD_PINS        8
#define KEYPAD_IO           {2, 3, 4, 5, 6, 7, 8, 9}
#define KEYPAD_MASK         (0b11111111u << 2u)
#define KEYPAD_ROWMASK      (0b00001111u << 2u)
#define KEYPAD_COLMASK      (0b11110000u << 2u)

// 7-SEG DISPLAY
#define DISP_7SEG_MASK      (0x7FF << 10u)

/* leah:
    volume:
        hysteresis on volume potentiometer - adc - 
        alarm vol ramp  - function
    dyanmic brightness adjustment - potentially pwm
*/

// GPS MODULE - uart
#define GPS_UART_TX         40
#define GPS_UART_RX         41

// VOLUME CONTROL
#define VOL_ADC_PIN         44

// DISPLAY (Maddie) - spi
#define SPI_DISP_PORT       spi1
#define SPI_DISP_CSn        29
#define SPI_DISP_SCK        30
#define SPI_DISP_TX         31

#define DISP_BACKLIT_PWM    38

// SD CARD
#define SD_SPI_PORT         spi0
#define SD_PIN_MISO         32
#define SD_PIN_CS           33
#define SD_PIN_SCK          34
#define SD_PIN_MOSI         35

// PWM AUDIO PLAYBACK
#define BASE_CLK            150000000u
#define AUDIO_PWM_INT_NUM   PWM_IRQ_WRAP_1
#define AUDIO_PWM_SLICE     10
#define AUDIO_PWM_PIN_L     36
#define AUDIO_PWM_PIN_H     37