#define NUM_OUT             4
#define NUM_IN              2
#define OUT_GPIOS           {22, 23, 24, 25}
#define IN_GPIOS            {21, 26}
#define OUT_MASK            ( 0b1111u  << 22u)
#define IN_MASK             (0b100001u << 21u)

#define LIGHTS_ON_BUTTON    21
#define LIGHTS_OFF_BUTTON   26

#define NUM_PAD_PINS        8
#define KEYPAD_IO           {2, 3, 4, 5, 6, 7, 8, 9}
#define KEYPAD_MASK         (0b11111111u << 2u)
#define KEYPAD_ROWMASK      (0b00001111u << 2u)
#define KEYPAD_COLMASK      (0b11110000u << 2u)

#define DISPLAY_MASK        (0x7FF << 10u)

#define CORE_NUM            *(uint32_t *) (SIO_BASE + SIO_CPUID_OFFSET)
#define wait_adc()          while ( !(adc_hw -> cs & ADC_CS_READY_BITS) )