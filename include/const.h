// Constants and macros to make our lives easier!

// In order to prevent pin and hardware peripheral conflicts
// betwee different parts of the project, state your pin choices
// and spi/pwm/adc channels here.

// For brevity, only put stuff that you're "claiming" for your functions.
// Put all other define statements in your specific header files!

#include <hardware/spi.h>
#include <hardware/timer.h>

// MACROS
#define GET_CORE_NUM()      *(uint32_t *) (SIO_BASE + SIO_CPUID_OFFSET)
#define ADC_WAIT()          while ( !(adc_hw -> cs & ADC_CS_READY_BITS) )
#define UART_WAIT()         while (!uart_is_writable(uart0)) { tight_loop_contents(); }
#define GPS_UART_WAIT()     while (!uart_is_writable(uart1)) { tight_loop_contents(); }

/*** FORMAT                         ***/
/*** FUNCTIONALITY - resources used ***/

// TIME
#define TIME_ZONE_OFFSET    -4
#define DAYLIGHT_SAVINGS_EN false

// ONBOARD - sio, unused
#define ONBOARD_LEDS        {22, 23, 24, 25}
#define ONBOARD_PUSHBUTTONS {21, 26}

// KEYPAD - sio
#define NUM_PAD_PINS        8
#define KEYPAD_IO           {2, 3, 4, 5, 6, 7, 8, 9}
#define KEYPAD_MASK         (0b11111111u << 2u)
#define KEYPAD_ROWMASK      (0b00001111u << 2u)
#define KEYPAD_COLMASK      (0b11110000u << 2u)

// 7-SEG DISPLAY - unused
#define DISP_7SEG_MASK      (0x7FF << 10u)

// GPS MODULE - uart1
#define GPS_UART_PORT       uart1
#define GPS_UART_IRQ        UART_IRQ_NUM(GPS_UART_PORT)
#define GPS_UART_INT_PRI    2
#define GPS_PIN_TX          24
#define GPS_PIN_RX          25

// VOLUME CONTROL - adc4
#define VOL_RAMP_TIMER_HW   timer0_hw
#define VOL_RAMP_TIM_ALARM  1
#define VOL_RAMP_INT_NUM    TIMER0_IRQ_1
#define VOL_RAMP_INT_PRI    1
#define VOL_ADC_PIN         45
#define VOL_DMA_CH          0

// DISPLAY (Maddie) - sio, spi1, pwm5a
#define DISP_RST            27
#define DISP_DCRS           28

#define DISP_SPI_PORT       spi1
#define DISP_PIN_CSn        29
#define DISP_PIN_SCK        30
#define DISP_PIN_TX         31

#define DISP_BACKLIT_PWM    26

// SD CARD - spi0
#define SD_SPI_PORT         spi0
#define SD_PIN_MISO         32
#define SD_PIN_CS           33
#define SD_PIN_SCK          34
#define SD_PIN_MOSI         35

// PWM AUDIO PLAYBACK - pwm10ab, pwm_irq_wrap_1
#define BASE_CLK            150000000u
#define AUDIO_PWM_INT_NUM   PWM_IRQ_WRAP_0
#define AUDIO_PWM_INT_PRI   0
#define AUDIO_PWM_SLICE     11
#define AUDIO_PWM_PIN_L     38
#define AUDIO_PWM_PIN_H     39

/* leah:
    volume:
        hysteresis on volume potentiometer - adc - 
        alarm vol ramp  - function
    dyanmic brightness adjustment - potentially pwm
*/