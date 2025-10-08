#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "const.h"
#include "queue.h"
#include "support.h"

//////////////////////////////////////////////////////////////////////////////

const char* username = "yu1286";

//////////////////////////////////////////////////////////////////////////////

#define BASE_CLK            150000000u
#define PWM_CLK_PSC         150u
#define AUDIO_SAMPLE_PERIOD (BASE_CLK / PWM_CLK_PSC / RATE)

static uint8_t duty_cycle = 0;
static uint8_t dir = 0;
static uint8_t color = 0;

void autotest();
void display_init_pins();
void display_init_timer();
void display_char_print(const char message[]);
void keypad_init_pins();
void keypad_init_timer();
void init_wavetable(void);
void set_freq(int chan, float f);
extern KeyEvents kev;

//////////////////////////////////////////////////////////////////////////////

// When testing static duty-cycle PWM
// #define STEP2
// When testing variable duty-cycle PWM
// #define STEP3
// When testing 8-bit audio synthesis
#define STEP4

//////////////////////////////////////////////////////////////////////////////

void init_pwm_static(uint32_t period, uint32_t duty_cycle);
void init_pwm_irq();
void pwm_breathing();
void pwm_audio_handler();
void init_pwm_audio();

void init_pwm_static(uint32_t period, uint32_t duty_cycle) {

    /**
     * 37: 10B
     * 38: 11A
     * 39: 11B
    */
    
    for (uint8_t gpio = 37; gpio <= 39; ++gpio) {
        // enable interaction with outside world
        hw_write_masked(&pads_bank0_hw -> io[gpio],
                    PADS_BANK0_GPIO0_IE_BITS,
                    PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
        );

        // set output mux to PWM
        io_bank0_hw -> io[gpio].ctrl = GPIO_FUNC_PWM << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;

        // diable isolation latches (not necessary for input)
        #if HAS_PADS_BANK0_ISOLATION
            // Remove pad isolation now that the correct peripheral is in control of the pad
            hw_clear_bits(&pads_bank0_hw -> io[gpio], PADS_BANK0_GPIO0_ISO_BITS);
        #endif
    }

    pwm_slice_hw_t * slice_10 = &(pwm_hw -> slice[10]);
    pwm_slice_hw_t * slice_11 = &(pwm_hw -> slice[11]);

    // Set psc 150
    slice_10 -> div = (PWM_CLK_PSC << PWM_CH10_DIV_INT_LSB);
    slice_11 -> div = (PWM_CLK_PSC << PWM_CH11_DIV_INT_LSB);

    // Config rollover value
    period &= PWM_CH0_TOP_BITS;
    slice_10 -> top = (slice_10 -> top & ~PWM_CH10_TOP_BITS) | (period - 1);
    slice_11 -> top = (slice_11 -> top & ~PWM_CH11_TOP_BITS) | (period - 1);
    
    // Config digital comparator threshold
    duty_cycle &= PWM_CH0_CC_A_BITS;
    slice_10 -> cc  = (duty_cycle << PWM_CH10_CC_B_LSB);
    slice_11 -> cc  = (duty_cycle << PWM_CH11_CC_B_LSB) | (duty_cycle << PWM_CH11_CC_A_LSB);

    // Enable PWM
    uint32_t csr_mask = (PWM_CH0_CSR_DIVMODE_VALUE_DIV << PWM_CH0_CSR_DIVMODE_LSB);
    slice_10 -> csr |= csr_mask;
    slice_11 -> csr |= csr_mask;

    // Enable PWM
    pwm_hw -> en    |= PWM_EN_CH10_BITS | PWM_EN_CH11_BITS;
}

void init_pwm_irq() {
    // Enable interrupt signal of slice 10
    pwm_hw -> inte  |= PWM_IRQ0_INTE_CH10_BITS;
    // Set handler to pwm_breathing
    irq_set_exclusive_handler(PWM_IRQ_WRAP_0, &pwm_breathing);
    // Enable IRQ repsonse
    irq_set_enabled(PWM_IRQ_WRAP_0, true);
    
    // Init duty cycles
    uint16_t current_period = (pwm_hw -> slice[10].top) + 1;
    duty_cycle              = 0;
    dir                     = 0;
    
    uint16_t cc_val         = ((uint32_t)duty_cycle * current_period) / 100u;

    pwm_hw -> slice[10].cc  = (cc_val << PWM_CH10_CC_B_LSB);
    pwm_hw -> slice[11].cc  = (cc_val << PWM_CH11_CC_B_LSB) | (cc_val << PWM_CH11_CC_A_LSB);
}

void pwm_breathing() {
    // Ack irq trigger
    pwm_hw -> intr  |= PWM_INTR_CH10_BITS;
    
    // Increment color mod 3s
    if      (duty_cycle == 0   && dir == 1 && ++color == 3) { color = 0; }
    // Swap directions after duty cycle is maxxed
    dir ^=  (duty_cycle == 0) | (duty_cycle == 100);
    // Increment in direction
    dir ? --duty_cycle : ++duty_cycle;

    // Set chosen color's duty cycle
    volatile uint32_t * cc_reg  = (color == 0) ? &(pwm_hw -> slice[10].cc) : &(pwm_hw -> slice[11].cc);
    uint8_t cc_lsb              = (color == 1) ? 0 : 16;
 
    uint16_t current_period     = (pwm_hw -> slice[10].top) + 1;
    uint16_t cc_val             = ((uint32_t)duty_cycle * current_period) / 100u;

    *cc_reg                     = (*cc_reg & ~(0xFFFF << cc_lsb)) | (cc_val << cc_lsb);

    // Debug ts pmo
    printf(
        "DUTY = %3d | DIR = %2d | COLOR = %2d | RGB =   %5d / %5d,   %5d / %5d,   %5d / %5d\n",
        duty_cycle, dir, color,
        (int)(pwm_hw -> slice[10].cc >> PWM_CH11_CC_B_LSB), (int)(pwm_hw -> slice[10].top),
        (int)(pwm_hw -> slice[11].cc & PWM_CH10_CC_A_BITS), (int)(pwm_hw -> slice[11].top),
        (int)(pwm_hw -> slice[11].cc >> PWM_CH11_CC_B_LSB), (int)(pwm_hw -> slice[11].top)
    );
}

void init_pwm_audio() {

    const uint8_t gpio = 36;
    // enable interaction with outside world
    hw_write_masked(&pads_bank0_hw -> io[gpio],
                PADS_BANK0_GPIO0_IE_BITS,
                PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );

    // set output mux to PWM
    io_bank0_hw -> io[gpio].ctrl = GPIO_FUNC_PWM << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;

    // diable isolation latches (not necessary for input)
    #if HAS_PADS_BANK0_ISOLATION
        // Remove pad isolation now that the correct peripheral is in control of the pad
        hw_clear_bits(&pads_bank0_hw -> io[gpio], PADS_BANK0_GPIO0_ISO_BITS);
    #endif

    pwm_slice_hw_t * slice_10 = &(pwm_hw -> slice[10]);

    // Set psc 150
    slice_10 -> div = (PWM_CLK_PSC << PWM_CH10_DIV_INT_LSB);

    // Config rollover value
    uint16_t period = AUDIO_SAMPLE_PERIOD;
    slice_10 -> top = (slice_10 -> top & ~PWM_CH10_TOP_BITS) | (period - 1);
    
    // Config digital comparator threshold
    // uint8_t duty    = 0;
    // uint16_t cc_val = ((uint32_t)duty * period) / 100u;
    // slice_10 -> cc  |= (cc_val << PWM_CH10_CC_A_LSB);

    // Config DIVMODE
    // uint32_t csr_mask = (PWM_CH0_CSR_DIVMODE_VALUE_DIV << PWM_CH0_CSR_DIVMODE_LSB);
    // slice_10 -> csr |= csr_mask;

    init_wavetable();

    // Enable interrupt signal of slice 10
    pwm_hw -> inte  |= PWM_IRQ0_INTE_CH10_BITS;
    // Set handler to pwm_breathing
    irq_set_exclusive_handler(PWM_IRQ_WRAP_0, &pwm_audio_handler);
    // Enable IRQ repsonse
    irq_set_enabled(PWM_IRQ_WRAP_0, true);
    
    // Enable PWM slice
    pwm_hw -> en    |= PWM_EN_CH10_BITS;
}

void pwm_audio_handler() {
    // Ack irq trigger
    pwm_hw -> intr  |= PWM_INTR_CH10_BITS;

    // Increment sample index
    offset0 += step0;
    offset1 += step1;

    // Handle modulus over wave table length
    if (offset0 >= (N << 16)) { offset0 -= (N << 16); }
    if (offset1 >= (N << 16)) { offset1 -= (N << 16); }

    // Mix two waves and determine cc value
    uint32_t samp   = ((uint32_t)wavetable[offset0 >> 16] + (uint32_t)wavetable[offset1 >> 16]) >> 1;
    uint16_t cc_val = ((uint32_t)samp * AUDIO_SAMPLE_PERIOD) >> 16;

    // Apply cc value
    pwm_slice_hw_t * slice_10 = &(pwm_hw -> slice[10]);
    slice_10 -> cc = (pwm_hw -> slice[10].cc & ~PWM_CH10_CC_A_BITS) | (cc_val & PWM_CH10_CC_A_BITS);
}

//////////////////////////////////////////////////////////////////////////////

int main()
{
    // Configures our microcontroller to 
    // communicate over UART through the TX/RX pins
    stdio_init_all();

    // Uncomment when you need to run autotest.
    // Keep this commented out until you need it
    // since it adds a lot of time to the upload process.
    // autotest();

    /*
    ****************************************************
    * Make sure to go through the code in these steps.  
    * A lot of it can be very useful for your projects.
    ****************************************************
    */
   
    #ifdef STEP2
    // Make sure to copy in the latest display.c and keypad.c from your previous labs.
    keypad_init_pins();
    keypad_init_timer();
    display_init_pins();
    display_init_timer();

    init_pwm_static(100, 50); // Start out with 500/1000, 50%
    display_char_print("      50");
    uint16_t percent = 50; // Set initial percentage for duty cycle, displayed 
    uint16_t disp_buffer = 0;
    char buf[9];

    // Display initial duty cycle
    snprintf(buf, sizeof(buf), "      50");
    display_char_print(buf);

    bool new_entry = true;  // Flag to track if we're starting a new entry
    
    for (;;) {
        uint16_t keyevent = key_pop(); // Pop a key event from the queue
        if (keyevent & 0x100) {
            char key = keyevent & 0xFF;
            if (key >= '0' && key <= '9') {
                // If the key is a digit, check if we need to clear the buffer first
                if (new_entry) {
                    disp_buffer = 0;  // Clear the buffer for new entry
                    new_entry = false;  // No longer a new entry
                }
                // Shift into buffer
                disp_buffer = (disp_buffer * 10) + (key - '0');
                snprintf(buf, sizeof(buf), "%8d", disp_buffer);
                display_char_print(buf); // Display the new value
            } else if (key == '#') {
                // If the key is '#', set the duty cycle
                percent = disp_buffer;
                if (percent > 100) {
                    percent = 100; // Cap at 100%
                }
                init_pwm_static(100, percent); // Update PWM with new duty cycle
                snprintf(buf, sizeof(buf), "%8d", percent);
                display_char_print(buf); // Display the new duty cycle
                new_entry = true;  // Ready for new entry
            }
            else if (key == '*') {
                // If the key is '*', reset the buffer
                disp_buffer = 50;
                percent = 50;
                init_pwm_static(100, percent); // Reset PWM to 50% duty cycle
                snprintf(buf, sizeof(buf), "      50");
                display_char_print(buf); // Display reset
                new_entry = true;  // Ready for new entry
            }
            else {
                // Any other key also starts a new entry
                new_entry = true;
            }
        }
    }
    #endif

    #ifdef STEP3
    init_pwm_static(10000, 5000); // Start out with 500/1000, 50%
    init_pwm_irq(); // Initialize PWM IRQ for variable duty cycle

    for(;;) {
        // The handler manages everything from now on.
        // Use the CPU to do something else!
        tight_loop_contents();
    }
    #endif
    
    #ifdef STEP4
    keypad_init_pins();
    keypad_init_timer();
    display_init_pins();
    display_init_timer();

    char freq_buf[9] = {0};
    int pos = 0;
    bool decimal_entered = false;
    int decimal_pos = 0;
    int current_channel = 0;

    init_pwm_audio(); 
    // set_freq(0, 440.0f); // Set initial frequency to 440 Hz (A4 note)
    // set_freq(1, 0.0f); // Turn off channel 1 initially
    // set_freq(0, 261.626f);
    // set_freq(1, 329.628f);

    set_freq(0, 440.0f); // Set initial frequency for channel 0
    display_char_print(" 440.000 ");

    for(;;) {
        uint16_t keyevent = key_pop();

        if (keyevent & 0x100) {
            char key = keyevent & 0xFF;
            if (key == 'A') {
                current_channel = 0;
                pos = 0;
                freq_buf[0] = '\0';
                decimal_entered = false;
                decimal_pos = 0;
                display_char_print("         ");
            } else if (key == 'B') {
                current_channel = 1;
                pos = 0;
                freq_buf[0] = '\0';
                decimal_entered = false;
                decimal_pos = 0;
                display_char_print("         ");
            } else if (key >= '0' && key <= '9') {
                if (pos == 0) {
                    snprintf(freq_buf, sizeof(freq_buf), "        "); // Clear buffer on first digit
                    display_char_print(freq_buf);
                }
                if (pos < 8) {
                    freq_buf[pos++] = key;
                    freq_buf[pos] = '\0';
                    display_char_print(freq_buf);
                    if (decimal_entered) decimal_pos++;
                }
                } else if (key == '*') {
                if (!decimal_entered && pos < 7) {
                    freq_buf[pos++] = '.';
                    freq_buf[pos] = '\0';
                    display_char_print(freq_buf);
                    decimal_entered = true;
                    decimal_pos = 0;
                }
                } else if (key == '#') {
                float freq = 0.0f;
                if (decimal_entered) {
                    freq = strtof(freq_buf, NULL);
                } else {
                    freq = (float)atoi(freq_buf);
                }
                set_freq(current_channel, freq);
                snprintf(freq_buf, sizeof(freq_buf), "%8.3f", freq);
                display_char_print(freq_buf);
                pos = 0;
                freq_buf[0] = '\0';
                decimal_entered = false;
                decimal_pos = 0;
            } else {
                // Reset on any other key
                pos = 0;
                freq_buf[0] = '\0';
                decimal_entered = false;
                decimal_pos = 0;
                display_char_print("        ");
            }
        }
    }
    #endif

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }

    for(;;);
    return 0;
}
