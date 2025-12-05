// Constants and macros to make our lives easier!

// In order to prevent pin and hardware peripheral conflicts
// between different parts of the project, state your pin choices
// and spi/pwm/adc channels here.

// For brevity, only put stuff that you're "claiming" for your functions.
// Put all other define statements in your specific header files!

#include <hardware/spi.h>
#include <hardware/timer.h>
#include <hardware/uart.h>

// MACROS
#define GET_CORE_NUM()      *(uint32_t *) (SIO_BASE + SIO_CPUID_OFFSET)
#define ADC_WAIT()          while ( !(adc_hw -> cs & ADC_CS_READY_BITS) )
#define UART_WAIT()         while (!uart_is_writable(uart0)) { tight_loop_contents(); }
#define GPS_UART_WAIT()     while (!uart_is_writable(uart1)) { tight_loop_contents(); }

/*** FORMAT                         ***/
/*** FUNCTIONALITY - resources used ***/

// ONBOARD - sio
#define ONBOARD_LEDS        {22, 23, 24, 25}
#define BRIGHT_PUSHB_PIN    21
#define VOL_PUSHB_PIN       26

// KEYPAD - sio
#define NUM_PAD_PINS        8
#define KEYPAD_IO           {2, 3, 4, 5, 6, 7, 8, 9}
#define KEYPAD_MASK         (0b11111111u << 2u)
#define KEYPAD_ROWMASK      (0b00001111u << 2u)
#define KEYPAD_COLMASK      (0b11110000u << 2u)

#define KEYPAD_TIMER_HW     timer0_hw
#define KEYPAD_COLSCAN_ALARM 0
#define KEYPAD_ROWSCAN_ALARM 1

// 7-SEG DISPLAY - unused
#define DISP_7SEG_MASK      (0x7FF << 10u)

// GPS MODULE - uart1
#define GPS_UART_PORT       uart1
#define GPS_UART_IRQ        UART_IRQ_NUM(GPS_UART_PORT)
#define GPS_UART_INT_PRI    2
#define GPS_PIN_TX          24
#define GPS_PIN_RX          25

// LISTEN IDK WHO DAFUQ IS USING TIMER0_IRQ_3 BUT IT'S TAKEN
// EVEN BEFORE MAIN IS CALLED AND WILL GIVE YOU A HARD FAULT
// DO NOT USE ITTTTTT 😡😡😡😡😡😡😡😡😡😡😡😡😡😡😡😡😡😡😡😡

// TIME UPDATES - timer0
#define TIME_STEP_TIMER_HW  timer1_hw
#define TIME_STEP_INT_NUM   TIMER1_IRQ_0
#define TIME_STEP_ALARM     0
#define TIME_STEP_INT_PRI   1

// VOLUME CONTROL - adc4, timer0
#define VOL_RAMP_TIMER_HW   timer1_hw
#define VOL_RAMP_INT_NUM    TIMER1_IRQ_1
#define VOL_RAMP_TIM_ALARM  1
#define VOL_RAMP_INT_PRI    3
#define KNOB_ADC_PIN        45
#define AMB_DMA_CH          0

// DISPLAY (Maddie) - sio, spi1
#define TFT_SPI             spi1
#define TFT_PIN_SCK         30

#define TFT_PIN_MOSI        31
#define TFT_PIN_CS          29
#define TFT_PIN_DC          28
#define TFT_PIN_RST         27

// DISPLAY BRIGHTNESS (Leah, Kevin) - pwm8a, adc4

#define TFT_BACKLIT_INT_NUM PWM_IRQ_WRAP_1
#define TFT_BACKLIT_INT_PRI 2
#define TFT_BACKLIT_SLICE   3
#define TFT_BACKLIT_PIN     23
#define TFT_BACKLIT_CH      (TFT_BACKLIT_PIN & 1)

#define LDR_ADC_PIN         44
#define LDR_DMA_CH          1

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