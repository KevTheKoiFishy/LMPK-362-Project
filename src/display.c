#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
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
    // GP2-GP5 as inputs (for reading the rows). It should initialize the column pins to 0.
    uint32_t mask_2 = 1u << (2 & 0x1fu);
    sio_hw->gpio_oe_clr = mask_2;
    sio_hw->gpio_clr = mask_2;
    hw_write_masked(&pads_bank0_hw->io[2],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    io_bank0_hw->io[2].ctrl = 5 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    hw_clear_bits(&pads_bank0_hw->io[2], PADS_BANK0_GPIO0_ISO_BITS);

    uint32_t mask_3 = 1u << (3 & 0x1fu);
    sio_hw->gpio_oe_clr = mask_3;
    sio_hw->gpio_clr = mask_3;
    hw_write_masked(&pads_bank0_hw->io[3],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    io_bank0_hw->io[3].ctrl = 5 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    hw_clear_bits(&pads_bank0_hw->io[3], PADS_BANK0_GPIO0_ISO_BITS);

    uint32_t mask_4 = 1u << (4 & 0x1fu);
    sio_hw->gpio_oe_clr = mask_4;
    sio_hw->gpio_clr = mask_4;
    hw_write_masked(&pads_bank0_hw->io[4],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    io_bank0_hw->io[4].ctrl = 5 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    hw_clear_bits(&pads_bank0_hw->io[4], PADS_BANK0_GPIO0_ISO_BITS);

    uint32_t mask_5 = 1u << (5 & 0x1fu);
    sio_hw->gpio_oe_clr = mask_5;
    sio_hw->gpio_clr = mask_5;
    hw_write_masked(&pads_bank0_hw->io[5],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    io_bank0_hw->io[5].ctrl = 5 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    hw_clear_bits(&pads_bank0_hw->io[5], PADS_BANK0_GPIO0_ISO_BITS);

    // Initialize pins GP10-GP20 as outputs.
    uint32_t mask_10 = 1u << (10 & 0x1fu);
    sio_hw->gpio_oe_set = mask_10;
    sio_hw->gpio_set = mask_10;
    hw_write_masked(&pads_bank0_hw->io[10],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    io_bank0_hw->io[10].ctrl = 5 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    hw_clear_bits(&pads_bank0_hw->io[10], PADS_BANK0_GPIO0_ISO_BITS);

    uint32_t mask_11 = 1u << (11 & 0x1fu);
    sio_hw->gpio_oe_set = mask_11;
    sio_hw->gpio_set = mask_11;
    hw_write_masked(&pads_bank0_hw->io[11],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    io_bank0_hw->io[11].ctrl = 5 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    hw_clear_bits(&pads_bank0_hw->io[11], PADS_BANK0_GPIO0_ISO_BITS);

    uint32_t mask_12 = 1u << (12 & 0x1fu);
    sio_hw->gpio_oe_set = mask_12;
    sio_hw->gpio_set = mask_12;
    hw_write_masked(&pads_bank0_hw->io[12],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    io_bank0_hw->io[12].ctrl = 5 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    hw_clear_bits(&pads_bank0_hw->io[12], PADS_BANK0_GPIO0_ISO_BITS);

    uint32_t mask_13 = 1u << (13 & 0x1fu);
    sio_hw->gpio_oe_set = mask_13;
    sio_hw->gpio_set = mask_13;
    hw_write_masked(&pads_bank0_hw->io[13],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    io_bank0_hw->io[13].ctrl = 5 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    hw_clear_bits(&pads_bank0_hw->io[13], PADS_BANK0_GPIO0_ISO_BITS);

    uint32_t mask_14 = 1u << (14 & 0x1fu);
    sio_hw->gpio_oe_set = mask_14;
    sio_hw->gpio_set = mask_14;
    hw_write_masked(&pads_bank0_hw->io[14],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    io_bank0_hw->io[14].ctrl = 5 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    hw_clear_bits(&pads_bank0_hw->io[14], PADS_BANK0_GPIO0_ISO_BITS);

    uint32_t mask_15 = 1u << (15 & 0x1fu);
    sio_hw->gpio_oe_set = mask_15;
    sio_hw->gpio_set = mask_15;
    hw_write_masked(&pads_bank0_hw->io[15],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    io_bank0_hw->io[15].ctrl = 5 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    hw_clear_bits(&pads_bank0_hw->io[15], PADS_BANK0_GPIO0_ISO_BITS);

    uint32_t mask_16 = 1u << (16 & 0x1fu);
    sio_hw->gpio_oe_set = mask_16;
    sio_hw->gpio_set = mask_16;
    hw_write_masked(&pads_bank0_hw->io[16],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    io_bank0_hw->io[16].ctrl = 5 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    hw_clear_bits(&pads_bank0_hw->io[16], PADS_BANK0_GPIO0_ISO_BITS);

    uint32_t mask_17 = 1u << (17 & 0x1fu);
    sio_hw->gpio_oe_set = mask_17;
    sio_hw->gpio_set = mask_17;
    hw_write_masked(&pads_bank0_hw->io[17],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    io_bank0_hw->io[17].ctrl = 5 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    hw_clear_bits(&pads_bank0_hw->io[17], PADS_BANK0_GPIO0_ISO_BITS);

    uint32_t mask_18 = 1u << (18 & 0x1fu);
    sio_hw->gpio_oe_set = mask_18;
    sio_hw->gpio_set = mask_18;
    hw_write_masked(&pads_bank0_hw->io[18],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    io_bank0_hw->io[18].ctrl = 5 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    hw_clear_bits(&pads_bank0_hw->io[18], PADS_BANK0_GPIO0_ISO_BITS);

    uint32_t mask_19 = 1u << (19 & 0x1fu);
    sio_hw->gpio_oe_set = mask_19;
    sio_hw->gpio_set = mask_19;
    hw_write_masked(&pads_bank0_hw->io[19],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    io_bank0_hw->io[19].ctrl = 5 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    hw_clear_bits(&pads_bank0_hw->io[19], PADS_BANK0_GPIO0_ISO_BITS);

    uint32_t mask_20 = 1u << (20 & 0x1fu);
    sio_hw->gpio_oe_set = mask_20;
    sio_hw->gpio_set = mask_20;
    hw_write_masked(&pads_bank0_hw->io[20],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    io_bank0_hw->io[20].ctrl = 5 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
    hw_clear_bits(&pads_bank0_hw->io[20], PADS_BANK0_GPIO0_ISO_BITS);
}

void display_isr() {
    // Acknowledge the interrupt for ALARM0 on TIMER1.
    hw_set_bits(&timer1_hw->inte, TIMER_INTE_ALARM_0_BITS);   
    
    // Set the value of GP20-GP10 to a new 11-bit value 
    // {GP20 ... GP10} = [digit_select(3 bits) : segments(8 bits)]  11-bit bus
    uint32_t bus11 = ((uint32_t)(index & 7) << 8) | (uint8_t)msg[index];

    // Align to GP10..GP20
    const uint32_t BUS_MASK = 0x7FFu << 10;
    uint32_t desired = (bus11 << 10) & BUS_MASK;

    // Force all bus pins low, then raise only those that should be high
    sio_hw->gpio_clr = BUS_MASK;
    sio_hw->gpio_set = desired;

    // Increment index by 1, and wrap around to 0 after 7. 
    index = (index + 1) & 7;   

    // Make TIMER1 ALARM0 fire the interrupt again in 3 milliseconds.
    timer1_hw->alarm[0] = timer1_hw->timerawl + 3000u;  // 3 ms = 3000 µs
    hw_clear_bits(&timer1_hw->intr, TIMER_INTR_ALARM_0_BITS); 
}

void display_init_timer() {
    // Initialize TIMER1 to fire ALARM0 after 3 milliseconds
    hw_set_bits(&timer1_hw->inte,  TIMER_INTE_ALARM_0_BITS);

    // at which point the interrupt will call display_isr when triggered.
    irq_set_exclusive_handler(TIMER1_IRQ_0, display_isr);

    // Make sure to enable the interrupt for ALARM0 on TIMER1.
    irq_set_enabled(TIMER1_IRQ_0, true);

    // Write the time you would like the interrupt to fire to ALARM
    uint32_t target = timer1_hw->timerawl;
    timer1_hw->alarm[0] = target + 3000u;  // 3 miliseconds  

    // Once the alarm has fired, the ARMED bit clears to 0. To clear the latched interrupt, write a 1 to the appropriate bit in INTR.
    hw_clear_bits(&timer1_hw->intr, TIMER_INTR_ALARM_0_BITS); 
}

void display_print(const uint16_t message[]) {
    // This takes an array of "key-events" from the keypad, transforms them into the seven-segment representation, 
     for (int i = 0; i < 8; ++i) {  // 7-segment digit
        uint16_t ev = message[i]; // bits7:0=ASCII of the key
        uint8_t ch = (uint8_t)(ev & 0xFF);  // Extract the lower 8 bits (ASCII value of the key) for font lookup
        uint8_t seg = (uint8_t)font[ch];  // Use the global font array to convert ASCII → 7-segment pattern (decimal point OFF by default)

        if (ev & (1u << 8)) {  // If the pressed state (9th bit) is 1, we need the decimal point ON for this character
            seg |= 0x80;  // Turn the decimal point ON by setting the MSB of the 7-segment byte (8'b1xxxxxxx)
        } else {  // Otherwise, the pressed state is 0 (released) so the decimal point must be OFF
            seg &= 0x7F;  // Turn the decimal point OFF by clearing the MSB (8'b0xxxxxxx)
        }

        msg[i] = (char)seg;  // Store the transformed 7-segment byte into the local msg array for display output
    } 
}