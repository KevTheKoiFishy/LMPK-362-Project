#include "pico/stdlib.h"
#include <hardware/gpio.h>
#include <stdio.h>
#include "keypad_queue.h"
#include "const.h"

// Global column variable
int col = -1;

// Global key state
static bool state[16] = {}; // Are keys pressed/released

// Keymap for the keypad
const char keymap[17] = "DCBA#9630852*741";

// Defined here to avoid circular dependency issues with autotest
// You can see the struct definition in queue.h

// KeyEvents kev = { 
//     .head = 0, 
//     .tail = 0 
// };

void keypad_drive_column();
void keypad_isr();

/********************************************************* */
// Implement the functions below.

void keypad_init_pins() {
    sio_hw -> gpio_oe_set = KEYPAD_COLMASK;
    sio_hw -> gpio_oe_clr = KEYPAD_ROWMASK;
    sio_hw -> gpio_clr    = KEYPAD_COLMASK;

    uint8_t keypad_gpios[NUM_PAD_PINS] = KEYPAD_IO;

    for (uint8_t i = 0; i < NUM_PAD_PINS; ++i) {
        uint32_t gpio = keypad_gpios[i];

        // enable interaction with outside world
        hw_write_masked(&pads_bank0_hw -> io[gpio],
                    PADS_BANK0_GPIO0_IE_BITS,
                    PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
        );

        // set output mux to SIO
        io_bank0_hw -> io[gpio].ctrl = GPIO_FUNC_SIO << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;

        // diable isolation latches (not necessary for input)
        #if HAS_PADS_BANK0_ISOLATION
            // Remove pad isolation now that the correct peripheral is in control of the pad
            hw_clear_bits(&pads_bank0_hw -> io[gpio], PADS_BANK0_GPIO0_ISO_BITS);
        #endif
    }
}

void keypad_init_timer() {
    timer0_hw -> inte |= TIMER_INTE_ALARM_0_BITS;
    irq_set_exclusive_handler(TIMER0_IRQ_0, keypad_drive_column);
    irq_set_enabled(TIMER0_IRQ_0, true);
    timer0_hw -> alarm[0] = timer0_hw -> timerawl + (int)1e6;

    timer0_hw -> inte |= TIMER_INTE_ALARM_1_BITS;
    irq_set_exclusive_handler(TIMER0_IRQ_1, keypad_isr);
    irq_set_enabled(TIMER0_IRQ_1, true);
    timer0_hw -> alarm[1] = timer0_hw -> timerawl + (int)1.1e6;
}

void keypad_drive_column() {
    timer0_hw -> intr |= TIMER_INTR_ALARM_0_BITS;

    if (col >= 3) { col = -1; }
    sio_hw -> gpio_clr = KEYPAD_COLMASK;
    sio_hw -> gpio_set = 0b1000000u << (++col);

    timer0_hw -> alarm[0] = timer0_hw -> timerawl + 2500;
}

uint8_t keypad_read_rows() {
    return ((sio_hw -> gpio_in) >> 2) & 0xF;
}

void keypad_isr() {
    timer0_hw -> intr |= TIMER_INTR_ALARM_1_BITS;
    
    uint8_t row_read = keypad_read_rows();
    uint8_t li = (col << 2);

    for (uint8_t row = 0; row < 4; ++row) {
        uint8_t state_now = (row_read & 1);
        if (state[li] != state_now) {
            key_push((state_now ? 0x100 : 0x0) | keymap[li]);
            state[li] = state_now;
        }

        ++li;
        row_read >>= 1;
    }

    timer0_hw -> alarm[1] = timer0_hw -> timerawl + 2500;
}
