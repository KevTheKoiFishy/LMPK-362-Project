#pragma once

// Constants and macros to make our lives easier
#include <hardware/spi.h>

// MACROS
#define GET_CORE_NUM()      *(uint32_t *) (SIO_BASE + SIO_CPUID_OFFSET)
#define ADC_WAIT()          while ( !(adc_hw -> cs & ADC_CS_READY_BITS) )

// ONBOARD
#define ONBOARD_LEDS        {22, 23, 24, 25}
#define ONBOARD_PUSHBUTTONS {21, 26}

// KEYPAD
#define KEYPAD_IO           {2, 3, 4, 5, 6, 7, 8, 9}
#define KEYPAD_MASK         (0b11111111u << 2u)
#define KEYPAD_ROWMASK      (0b00001111u << 2u)
#define KEYPAD_COLMASK      (0b11110000u << 2u)

// 7-SEG DISPLAY
#define DISP_7SEG_MASK      (0x7FF << 10u)

// SD CARD
#define SD_SPI_BAUD         1 * 1000 * 1000  // 1 MHz
#define SD_SPI_PORT         spi0
#define SD_PIN_MISO         16
#define SD_PIN_CS           17
#define SD_PIN_SCK          18
#define SD_PIN_MOSI         19

// PWM AUDIO PLAYBACK
#define BASE_CLK            150000000u
#define PWM_CLK_PSC         150u
#define AUDIO_SAMPLE_PERIOD (BASE_CLK / PWM_CLK_PSC / RATE)