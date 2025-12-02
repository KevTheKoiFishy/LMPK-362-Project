#include <stdio.h>
#include <string.h>
#include <math.h>
#include "control.h"
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

uint32_t adc_fifo_out = 0;
void display_init_pins();
void display_init_timer();
void display_char_print(const char* buffer);

volatile knobMode knob_mode = BRIGHTNESS_MODE;
volatile bool button_ever_pressed = false;
volatile bool start_ramp = false;  

static void init_buttons(void) {
    gpio_set_dir(21, GPIO_IN); // button to control brightness
    gpio_put(21, 0);
    gpio_set_function(21, 5);

    gpio_set_dir(26, GPIO_IN); // button to control volume
    gpio_put(26, 0);
    gpio_set_function(26, 5);
}

void gpio_isr() {    
    if (gpio_get_irq_event_mask(21) & GPIO_IRQ_EDGE_RISE) {
            gpio_acknowledge_irq(21, GPIO_IRQ_EDGE_RISE);
            knob_mode = BRIGHTNESS_MODE;  
            button_ever_pressed = true;          
    }
    if (gpio_get_irq_event_mask(26) & GPIO_IRQ_EDGE_RISE) {   
            gpio_acknowledge_irq(26, GPIO_IRQ_EDGE_RISE);
            if (knob_mode == VOLUME_MODE) {
            start_ramp = true; 
        } else {
            knob_mode = VOLUME_MODE;
        }
        button_ever_pressed = true;
    }
}

void init_gpio_irq() {
    // add the handler for both pins at once, enable the GPIO IRQ for both pins and BANK0 IRQ interrupt.
    uint32_t mask = (1u << 21) | (1u << 26);
    gpio_add_raw_irq_handler_masked(mask, gpio_isr);

    // configure GP21 and GP26 to a rising edge 
    gpio_set_irq_enabled(21, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(26, GPIO_IRQ_EDGE_RISE, true);

    // enable the top-level GPIO Bank 0 IRQ line 
    irq_set_enabled(IO_IRQ_BANK0, true);
}

void init_adc_freerun() {
    adc_init();
    adc_gpio_init(45);
    adc_select_input(5);
    adc_read();

    // start freerunning conversions on the ADC after enabling it.
    hw_set_bits(&adc_hw->cs, ADC_CS_START_MANY_BITS);
}

void init_dma() {
    // Configure DMA Channel 0 to read from the ADC FIFO and write to the variable adc_fifo_out. 
    dma_channel_hw_addr(0)->read_addr = (uint32_t) &adc_hw->fifo;
    dma_channel_hw_addr(0)->write_addr = (uint32_t) &adc_fifo_out;
    
    // Set the transfer count, or TRANS_COUNT, to the arithmetic OR of two values.
    dma_channel_hw_addr(0)->transfer_count = (1u << 28) | 1u;

    // Initialize the control trigger register of the DMA to zero.
    dma_hw->ch[0].ctrl_trig = 0;

    // Create a uint32_t variable called temp
    u_int32_t temp = 0;

    // OR in the following:
    // left-shift the value 1 by the correct number of bits 
    // that would correspond to the DATA_SIZE field of the DMA channel 0 configuration, 
    // DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_HALFWORD _u(0x1)
    // but you assign it to temp first.
    temp |= (DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_HALFWORD << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB);

    // Specify the ADC DREQ signal as the TREQ trigger.
    temp |= (DREQ_ADC << 17); 
    
    // set the EN bit
    temp |= 1u;

    // Write the temp variable to the dma_hw->ch[0].ctrl_trig register.
    dma_hw->ch[0].ctrl_trig = temp;
}

void init_adc_dma() {
    // call init_dma
    init_dma();

    // Call init_adc_freerun  
    init_adc_freerun();

    // Enable the ADC FIFO to store the generated samples.
    // Configure the ADC to send out a DREQ signal whenever a new sample is ready in the FIFO. 
    adc_fifo_setup(1, 1, 0, 0, 0);
}

// Alarm volume ramp-up logic
static float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static float volume_curve(float x) {
    x = clamp01(x);
    float k = 2.0f;      
    return powf(x, k);    // y = x^k
}

static float clamp_pct(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 100.0f) return 100.0f;
    return x;
}


void set_volume_percent(float pct) {
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    printf("[VOLUME] %.1f%%\n", pct);
}

static void alarm_volume_ramp_blocking(float start_pct, float end_pct, uint32_t ramp_ms, uint32_t step_ms)
{
    if (step_ms == 0) step_ms = 50;
    if (ramp_ms == 0) ramp_ms = 1;

    start_pct = clamp_pct(start_pct);
    end_pct   = clamp_pct(end_pct);

    uint32_t steps = ramp_ms / step_ms;
    if (steps == 0) steps = 1;

    printf("Starting alarm volume ramp: %.1f%% -> %.1f%% (%lu ms)\n", start_pct, end_pct, ramp_ms);

    for (uint32_t i = 0; i <= steps; i++) {
        float t = (float)i / (float)steps;   // 0 to 1
        float curved = volume_curve(t);     
        float current_pct = start_pct + (end_pct - start_pct) * curved;

        set_volume_percent(current_pct);
        sleep_ms(step_ms);
    }

    printf("Alarm volume ramp done.\n");
}

//////////////////////////////////////////////////////////////////////////////

void control_update (void) {   
    stdio_init_all();
    sleep_ms(2000);   // important: give USB time so prints show

    display_init_pins();
    display_init_timer();
    init_adc_dma();
    init_buttons();
    init_gpio_irq();

    bool ramp_done = false;   
    
    for (;;) {
        // read the latest knob value from ADC
        float v   = (adc_fifo_out * 3.3f) / 4095.0f;   // volts
        float pct = (v / 3.3f) * 100.0f;               // 0–100%

        // test ramp
        if (start_ramp && !ramp_done) {
            start_ramp = false;       // consume the request

            float target_volume = pct;  // ramp up to current knob position

            printf("\nStarting volume ramp to %.1f%%...\n", target_volume);
            fflush(stdout);

            // 7 seconds total, step every 100 ms
            alarm_volume_ramp_blocking(0.0f, target_volume, 7000, 100);

            printf("Ramp done.\n");
            fflush(stdout);

            //ramp_done = true;        // just once per run
        }

        //  knob level test per mode
        if (knob_mode == BRIGHTNESS_MODE) {
            printf("\rBrightness: %.1f %% (V = %.2f)   ", pct, v);
        } else { // VOLUME_MODE
            printf("\rVolume:     %.1f %% (V = %.2f)   ", pct, v);
        }

        fflush(stdout);
        sleep_ms(100); 
    }
}