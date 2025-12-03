#include "const.h"
#include "ambience_control.h"

//// VOLUME & VOLUME RAMP ////
uint16_t volume_adc_out     = 4095;
float    volume_scalar      = 1.0f;

uint16_t volume_ramp_delT   = -1;
float    volume_ramp_r      = -1.0f;
float    volume_ramp_delR   = -1.0f;
float    volume_ramp_a0     = -1.0f;
float    volume_ramp_af     = -1.0f;
bool     volume_ramp_en     = false;

uint16_t get_volume_adc()       { return volume_adc_out; }
float    get_volume_scalar()    { return volume_scalar;  }
bool     get_volume_ramp_en()   { return volume_ramp_en; }

//// GPIO ////

volatile knobMode knob_mode = BRIGHTNESS_MODE;
volatile bool button_ever_pressed = false;
volatile bool start_ramp = false;  

//// GPIO ////

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

//// GET ADC CONVERSIONS ////

// Start freerunning conversions for volume knob
void init_volume_adc_freerun() {
    adc_init();
    adc_gpio_init(VOL_ADC_PIN);
    adc_select_input(VOL_ADC_PIN - 40);
    adc_read();

    hw_set_bits(&adc_hw->cs, ADC_CS_START_MANY_BITS);
}

// Initialize volume DMA Channel
void init_volume_dma() {
    dma_channel_hw_t * volume_dma_ch = &dma_hw -> ch[VOL_DMA_CH];

    // Configure DMA Channel 0 to read from the ADC FIFO and write to the variable "volume". 
    volume_dma_ch -> read_addr  = (uint32_t) &adc_hw->fifo;
    volume_dma_ch -> write_addr = (uint32_t) &volume_adc_out;
    
    // Set the transfer count, or TRANS_COUNT, to the arithmetic OR of two values.
    volume_dma_ch -> transfer_count = (1u << 28) | 1u;

    // Initialize the control trigger register of the DMA to zero.
    volume_dma_ch -> ctrl_trig = 0;

    // Create a uint32_t variable called temp
    uint32_t temp = 0;

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
    volume_dma_ch -> ctrl_trig = temp;
}

// Initialize automatic conversions and volume_dial update.
void init_volume_adc_and_dma() {
    // Start DMA
    init_volume_dma();

    // Start ADC
    init_volume_adc_freerun();

    // Enable the ADC FIFO to store the generated samples.
    // Configure the ADC to send out a DREQ signal whenever a new sample is ready in the FIFO. 
    adc_fifo_setup(1, 1, 0, 0, 0);
}

//// VOLUME MODULATION ////

// Clamp between [0, 1]
static float clamp01(float x) {
    if (x < 0.0f)   return 0.0f;
    if (x > 1.0f)   return 1.0f;
    return x;
}

// Clamp between [0, 100]
static float clamp_pct(float x) {
    if (x < 0.0f)   return 0.0f;
    if (x > 100.0f) return 100.0f;
    return x;
}

// Alarm volume ramp-up fxn: Makes the ramp start gentle and increase faster near the end.
static float volume_curve(float x) {
    x = clamp01(x);
    float k = VOLUME_RAMP_POW_CURVE_EXP;
    return powf(x, k);    // y = x^k
}

// Execute volume_curve function. Ramps volume between start pct and end pct of the dial setting.
void alarm_volume_ramp_blocking(float start_pct, float end_pct, uint16_t ramp_ms, uint16_t step_ms)
{
    if (step_ms <= 0) step_ms = VOLUME_RAMP_DEFAULT_STEP_MS;
    if (ramp_ms <= 0) ramp_ms = VOLUME_RAMP_DEFAULT_RAMP_MS;

    start_pct = clamp_pct(start_pct);
    end_pct   = clamp_pct(end_pct);

    uint16_t steps = ramp_ms / step_ms;
    if (steps == 0) steps = 1;

    printf("Starting alarm volume ramp: %.1f%% -> %.1f%% (%ld ms)\n", start_pct, end_pct, ramp_ms);

    for (uint16_t i = 0; i <= steps; i++) {
        float t = (float)i / (float)steps;   // 0 to 1
        float curved = volume_curve(t);     
        float current_pct = start_pct + (end_pct - start_pct) * curved;

        // set volume
        set_audio_volume(current_pct * 0.01f);
        sleep_ms(step_ms);
    }

    printf("Alarm volume ramp done.\n");
}

void volume_ramp_isr() {
    // Ack
    VOL_RAMP_TIMER_HW -> intr |= 1 << VOL_RAMP_TIM_ALARM;

    // Set volume via curve
    volume_scalar = volume_ramp_a0 + (volume_ramp_af - volume_ramp_a0) * volume_curve(volume_ramp_r);

    // Update ramp progress
    volume_ramp_r += volume_ramp_delR;
    if (volume_ramp_r > 1) { disable_volume_ramp_int(); return; }
    
    // Set next isr time.
    // !!! ENSURE ALARMS ARE SET RELATIVE TO BASE OR PREVIOUS TIME, NOT TIMERAWL AT TIME OF RUN.
    VOL_RAMP_TIMER_HW -> alarm[VOL_RAMP_TIM_ALARM] += volume_ramp_delT * 1000;
}

void configure_volume_ramp_int(float start_ratio, float end_ratio, uint16_t ramp_ms, uint16_t step_ms) {
    // Nope!
    if (step_ms <= 0) step_ms = VOLUME_RAMP_DEFAULT_STEP_MS;
    if (ramp_ms <= 0) ramp_ms = VOLUME_RAMP_DEFAULT_RAMP_MS;

    volume_ramp_a0      = clamp_01(start_ratio);
    volume_ramp_af      = clamp_01(end_ratio);

    // Track progress and isr fire times
    volume_ramp_delT    = step_ms;
    volume_ramp_r       = 0.f;
    volume_ramp_delR    = (float)step_ms / ramp_ms;
    
    // Set up isr
    irq_set_exclusive_handler(VOL_RAMP_INT_NUM, &volume_ramp_isr);
    irq_set_priority(VOL_RAMP_INT_NUM, VOL_RAMP_INT_PRI);
}

void enable_volume_ramp_int() {
    // Set up initial trigger time
    VOL_RAMP_TIMER_HW -> alarm[VOL_RAMP_TIM_ALARM] = VOL_RAMP_TIMER_HW -> timerawl + volume_ramp_delT * 1000;
    printf("Starting alarm volume ramp: %.1f%% -> %.1f%% by %ld ms\n", volume_ramp_a0*100.f, volume_ramp_af*100.f, volume_ramp_delT);

    // Enable interrupt
    VOL_RAMP_TIMER_HW -> inte |= (1 << VOL_RAMP_TIM_ALARM);
    irq_set_enabled(VOL_RAMP_INT_NUM, true);
}

void disable_volume_ramp_int() {
    // Disable Interrupt
    VOL_RAMP_TIMER_HW -> inte &= ~(1 << VOL_RAMP_TIM_ALARM);
    irq_set_enabled(VOL_RAMP_INT_NUM, false);
}

//// DISPLAY NIGHT MODE ////

// Handles ranges that wrap midnight
static bool is_night_time(int hour, const NightModeConfig *cfg) {
    if (!cfg) return false;
    if (hour < 0)  { hour = 0;  }
    if (hour > 23) { hour = 23; }

    int start = cfg -> night_start_hour;
    int end   = cfg -> night_end_hour;

    if (start <= end) {
        // Non-wrapping case like 20 to 23
        return (hour >= start && hour < end);
    } else {
        // Wrapping case like 23 to 6
        return (hour >= start || hour < end);
    }
}

// returns the clamped brightness that was actually applied
float apply_night_mode_brightness(float knob_pct, int current_hour_24, const NightModeConfig *cfg) {
    if (!cfg) return knob_pct;

    knob_pct = clamp_pct(knob_pct);

    float max_bri = is_night_time(current_hour_24, cfg) ? cfg->night_max_brightness : cfg->day_max_brightness;

    max_bri = clamp_pct(max_bri);

    if (knob_pct > max_bri) {
        knob_pct = max_bri;
    }

    set_brightness_percent(knob_pct);  // send to “hardware”
    return knob_pct;
}

//////////////////////////////////////////////////////////////////////////////
/*
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
}*/