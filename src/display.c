#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "const.h"

// 7-segment display message buffer
// Declared as static to limit scope to this file only.
static char msg[8] = {
    0x3F, // seven-segment value of 0
    0x06, // seven-segment value of 1
    0x5B, // seven-segment value of 2
    0x4F, // seven-segment value of 3
    0x66, // seven-segment value of 4
    0x6D, // seven-segment value of 5
    0x7D, // seven-segment value of 6
    0x07, // seven-segment value of 7
};

extern char font[]; // Font mapping for 7-segment display
static int index = 0; // Current index in the message buffer

// We provide you with this function for directly displaying characters.
// However, it can't use the decimal point, which display_print does.
void display_char_print(const char message[]) {
    for (int i = 0; i < 8; i++) {
        msg[i] = font[message[i] & 0xFF];
    }
}

/********************************************************* */
// Implement the functions below.


void display_init_pins() {
    sio_hw -> gpio_oe_set = sio_hw -> gpio_clr = DISPLAY_MASK;

    for (uint8_t gpio = 10; gpio < 21; ++gpio) {

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

void display_isr() {
    timer1_hw -> intr |= TIMER_INTR_ALARM_0_BITS;

    sio_hw -> gpio_clr = DISPLAY_MASK;
    sio_hw -> gpio_set = (index << 18u) | ((msg[index] & 0xFF) << 10u);
    index = (index + 1) & 0x7;

    timer1_hw -> alarm[0] = timer1_hw -> timerawl + (int)1e3;
}

void display_init_timer() {
    timer1_hw -> inte |= TIMER_INTE_ALARM_0_BITS;
    irq_set_exclusive_handler(TIMER1_IRQ_0, display_isr);
    irq_set_enabled(TIMER1_IRQ_0, true);
    timer1_hw -> alarm[0] = timer1_hw -> timerawl + (int)3e3;
}

void display_print(const uint16_t kevs[]) {
    for (int i = 0; i < 8; i++) {
        msg[i] = font[kevs[i] & 0xFF] | ((kevs[i] & 0x100) >> 1);
    }
}