#include "const.h"
#include "ambience_control.h"

//// GPIO ////
volatile knobMode knob_mode = BRIGHTNESS_MODE;
knobMode    get_knob_mode() { return knob_mode; }

//// ADC DMA DESTINATION ////
uint16_t adc_outs[ADC_ROUNDROBIN_BUF_LEN] = {4095, 0}; // Knob, LDR

//// VOLUME & VOLUME RAMP ////
dma_channel_hw_t * amb_dma_channel = &dma_hw -> ch[AMB_DMA_CH]; // Will use for BOTH volume and Brightness!

uint16_t *  volume_adc_out     = &adc_outs[0];
uint16_t    prev_volume_adc_out = 4095;
float       volume_scalar      = 1.0f;

uint16_t    volume_ramp_tF     = -1;
uint16_t    volume_ramp_delT   = -1;
float       volume_ramp_r      = -1.0f;
float       volume_ramp_delR   = -1.0f;
float       volume_ramp_a0     = -1.0f;
float       volume_ramp_af     = -1.0f;
bool        volume_ramp_en     = true;

uint16_t    get_volume_setting()    { return knob_mode == VOLUME_MODE ? *volume_adc_out : prev_volume_adc_out; }
float       get_volume_scalar()     { return volume_scalar;  }
bool        get_volume_ramp_en()    { return volume_ramp_en; }

//// BRIGHTNESS ////
dma_channel_hw_t * LDR_dma_ch = &dma_hw -> ch[LDR_DMA_CH];
float       brightness_curr_val         = 0.0f;
uint16_t *  brightness_set_adc_out      = &adc_outs[0];
uint16_t    prev_brightness_set_adc_out = 0;
uint16_t *  brightness_env_adc_out      = &adc_outs[1];
bool        brightness_adapt_en         = true;

uint16_t    get_brightness_setting() { return knob_mode == BRIGHTNESS_MODE ? *brightness_set_adc_out : prev_brightness_set_adc_out; }
uint16_t    get_brightness_env()     { return *brightness_env_adc_out; }
float       get_brightness_valf()    { return brightness_curr_val; }
bool        get_brightness_adapt_en(){ return brightness_adapt_en; }

//// GPIO ////

void init_buttons(void) {
    gpio_set_dir(BRIGHT_PUSHB_PIN, GPIO_IN); // button to control brightness
    gpio_put(BRIGHT_PUSHB_PIN, 0);
    gpio_set_function(BRIGHT_PUSHB_PIN, GPIO_FUNC_SIO);

    gpio_set_dir(VOL_PUSHB_PIN, GPIO_IN); // button to control volume
    gpio_put(VOL_PUSHB_PIN, 0);
    gpio_set_function(VOL_PUSHB_PIN, GPIO_FUNC_SIO);
}

void buttons_isr() {
    // Brightness Button 
    if (gpio_get_irq_event_mask(BRIGHT_PUSHB_PIN) & GPIO_IRQ_EDGE_FALL) {
        gpio_acknowledge_irq(BRIGHT_PUSHB_PIN, GPIO_IRQ_EDGE_FALL);

        // Toggle Adaptive Brightness
        if (knob_mode == BRIGHTNESS_MODE) {
            brightness_adapt_en = !brightness_adapt_en;
        }
        // Set knob to control brightness
        else {
            prev_volume_adc_out = *volume_adc_out;
            knob_mode = BRIGHTNESS_MODE;
        }
    }

    // Volume Button
    if (gpio_get_irq_event_mask(VOL_PUSHB_PIN) & GPIO_IRQ_EDGE_FALL) {   
        gpio_acknowledge_irq(VOL_PUSHB_PIN, GPIO_IRQ_EDGE_FALL);

        // Toggle volume ramping
        if (knob_mode == VOLUME_MODE) {
            volume_ramp_en = !volume_ramp_en;
        }
        // Switch to volume mode
        else {
            prev_brightness_set_adc_out = *brightness_set_adc_out;
            knob_mode = VOLUME_MODE;
        }
    }
}

void init_buttons_irq() {
    // add the handler for both pins at once, enable the GPIO IRQ for both pins and BANK0 IRQ interrupt.
    uint32_t mask = (1u << BRIGHT_PUSHB_PIN) | (1u << VOL_PUSHB_PIN);
    gpio_add_raw_irq_handler_masked(mask, &buttons_isr);

    // configure GP21 and GP26 to a rising edge
    gpio_set_irq_enabled(BRIGHT_PUSHB_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(VOL_PUSHB_PIN,    GPIO_IRQ_EDGE_FALL, true);

    // enable the top-level GPIO Bank 0 IRQ line 
    irq_set_enabled(IO_IRQ_BANK0, true);
}

//// ADC CONVERSIONS - KNOB ////

// Start round robin freerunning conversions
static void init_ambience_adc_freerun() {
    // Initialize ADC and Pins
    adc_init();
    adc_gpio_init(KNOB_ADC_PIN);
    adc_gpio_init(LDR_ADC_PIN);

    // Initial Channel = Knob
    adc_select_input(KNOB_ADC_PIN - 40);

    // Enable FIFO, Enable DREQ, DREQ threshold is 1, Disable Err Logging, Disable byte shift
    adc_fifo_setup(true, true, 1, false, false);

    // Take turns converting LDR and Knob
    adc_set_round_robin((1 << (KNOB_ADC_PIN - 40)) | (1 << (LDR_ADC_PIN - 40)));

    // Round Robin Mode
    hw_set_bits(&adc_hw->cs, ADC_CS_START_MANY_BITS);
}

// Initialize volume DMA Channel to trigger each time all sample conversions finish
static void init_ambience_dma() {

    // dma_channel_config_t ctrl_trig_cfg = { .ctrl = 0 };
    // channel_config_set_read_increment(&ctrl_trig_cfg, false);
    // channel_config_set_write_increment(&ctrl_trig_cfg, true);
    // dma_channel_configure(AMB_DMA_CH, &ctrl_trig_cfg, adc_outs, &adc_hw->fifo, ADC_ROUNDROBIN_BUF_LEN, false);

    // return;

    // Configure DMA Channel 0 to read from the ADC FIFO and write to the variable "volume". 
    amb_dma_channel -> read_addr  = (uint32_t) &adc_hw->fifo;
    amb_dma_channel -> write_addr = (uint32_t) &adc_outs;
    
    // Set the transfer count, or TRANS_COUNT, to the arithmetic OR of two values.
    amb_dma_channel -> transfer_count = (0x1 << DMA_CH0_TRANS_COUNT_MODE_LSB) | 1u; // 1 transfers for each DREQ. Do not decrement transfer count.

    // Initialize the control trigger register of the DMA to zero.
    amb_dma_channel -> ctrl_trig = 0;

    // Create a uint32_t variable for control trig
    uint32_t ctrl_trig_val = 0;

    // Halfword (16 bit) Write
    ctrl_trig_val |= (DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_HALFWORD << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB);

    // Ring Write
    ctrl_trig_val |= DMA_CH0_CTRL_TRIG_RING_SEL_BITS;                            // Ring write, not read
    ctrl_trig_val |= ADC_ROUNDROBIN_BUF_LEN << DMA_CH0_CTRL_TRIG_RING_SIZE_LSB;  // Ring size of 2
    ctrl_trig_val |= DMA_CH0_CTRL_TRIG_INCR_WRITE_BITS;                          // Increment write address by 2 bytes
 
    // Specify the ADC DREQ signal as the TREQ trigger.
    ctrl_trig_val |= (DREQ_ADC << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB);
    
    // Set the EN bit
    ctrl_trig_val |= 1u;

    // Write the temp variable to the dma_hw->ch[0].ctrl_trig register.
    amb_dma_channel -> ctrl_trig = ctrl_trig_val;
}

// Initialize automatic conversions and volume_dial update.
void init_ambience_adc_and_dma() {
    // Start dma BEFORE adc so it is ready for every ADC_DREQ
    init_ambience_dma();
    init_ambience_adc_freerun();
}

//// VOLUME MODULATION -- HELPERS ////

// Clamp between [0, 1]
static float clamp_01(float x) {
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
    x = clamp_01(x);
    float k = VOLUME_RAMP_POW_CURVE_EXP;
    return powf(x, k);    // y = x^k
}

//// VOLUME MODULATION -- BLOCKING ////

// Execute volume_curve function. Ramps volume between start pct and end pct of the dial setting.
void alarm_volume_ramp_blocking(float start_pct, float end_pct, uint16_t ramp_ms, uint16_t step_ms)
{
    if (step_ms <= 0) step_ms = VOLUME_RAMP_DEFAULT_STEP_MS;
    if (ramp_ms <= 0) ramp_ms = VOLUME_RAMP_DEFAULT_RAMP_MS;

    start_pct = clamp_pct(start_pct);
    end_pct   = clamp_pct(end_pct);

    uint16_t steps = ramp_ms / step_ms;
    if (steps == 0) steps = 1;

    printf("Starting alarm volume ramp: %.1f%% -> %.1f%% (%d ms)\n", start_pct, end_pct, ramp_ms);

    for (uint16_t i = 0; i <= steps; i++) {
        float t = (float)i / (float)steps;   // 0 to 1
        float curved = volume_curve(t);     
        float current_pct = start_pct + (end_pct - start_pct) * curved;

        // set volume
        volume_scalar = current_pct * 0.01f;
        sleep_ms(step_ms);
    }

    printf("Alarm volume ramp done.\n");
}

//// VOLUME MODULATION -- SCHEDULED ////

// Update volume_scalar by ramping rules.
void volume_ramp_isr() {
    // Ack
    VOL_RAMP_TIMER_HW -> intr |= 1 << VOL_RAMP_TIM_ALARM;

    // Set volume via curve
    volume_scalar = volume_ramp_a0 + (volume_ramp_af - volume_ramp_a0) * volume_curve(volume_ramp_r);

    // Update ramp progress
    volume_ramp_r += volume_ramp_delR;
    if (volume_ramp_r > 1.0f) { disable_volume_ramp_int(); return; }
    
    // Set next isr time.
    // !!! ENSURE ALARMS ARE SET RELATIVE TO BASE OR PREVIOUS TIME, NOT TIMERAWL AT TIME OF RUN.
    VOL_RAMP_TIMER_HW -> alarm[VOL_RAMP_TIM_ALARM] += volume_ramp_delT * 1000;
}

// Initialize ramp rules.
void configure_volume_ramp_int(float start_ratio, float end_ratio, uint16_t ramp_ms, uint16_t step_ms) {
    // Nope!
    if (step_ms <= 0) step_ms = VOLUME_RAMP_DEFAULT_STEP_MS;
    if (ramp_ms <= 0) ramp_ms = VOLUME_RAMP_DEFAULT_RAMP_MS;

    volume_ramp_a0      = clamp_01(start_ratio);
    volume_ramp_af      = clamp_01(end_ratio);

    // Track progress and isr fire times
    volume_ramp_tF      = ramp_ms;
    volume_ramp_delT    = step_ms;
    volume_ramp_r       = 0.f;
    volume_ramp_delR    = (float)step_ms / ramp_ms;
    
    // Set up isr
    irq_set_exclusive_handler(VOL_RAMP_INT_NUM, &volume_ramp_isr);
    irq_set_priority(VOL_RAMP_INT_NUM, VOL_RAMP_INT_PRI);
}

// Start volume ramp - start_audio_playback will be responsible for triggering when get_volume_ramp_en() is true;
void enable_volume_ramp_int() {
    // Set up initial trigger time
    VOL_RAMP_TIMER_HW -> alarm[VOL_RAMP_TIM_ALARM] = VOL_RAMP_TIMER_HW -> timerawl + volume_ramp_delT * 1000;
    printf("\n  Starting alarm volume ramp: %.1f%% -> %.1f%% over %d ms by %d ms\n", volume_ramp_a0*100.f, volume_ramp_af*100.f, volume_ramp_tF, volume_ramp_delT);

    // Enable interrupt
    VOL_RAMP_TIMER_HW -> inte |= (1 << VOL_RAMP_TIM_ALARM);
    irq_set_enabled(VOL_RAMP_INT_NUM, true);
}

// End volume ramp
void disable_volume_ramp_int() {
    // Disable Interrupt
    VOL_RAMP_TIMER_HW -> inte &= ~(1 << VOL_RAMP_TIM_ALARM);
    irq_set_enabled(VOL_RAMP_INT_NUM, false);
}

//// DISPLAY SET BRIGHTNESS ////

static float adaptive_brightness_activation(float x) {
    x = (x - 2048.f) * .0025f;          // Normalize Brightness
    return 4000.f / (1.0f + expf(x));   // Min Brightness is 95/4095
}

void display_brightness_isr() {
    pwm_hw -> intr  |= (1 << TFT_BACKLIT_SLICE);
    
    float new_brightness;

    if (brightness_adapt_en) {
        // Use LDR
        new_brightness = 0.9999f * brightness_curr_val + 0.0001f * adaptive_brightness_activation((float)(*brightness_env_adc_out));
    } else {
        // Use Knob
        new_brightness = 0.999f  * brightness_curr_val + 0.001f  * get_brightness_setting();
    }

    pwm_set_chan_level(TFT_BACKLIT_SLICE, TFT_BACKLIT_CH, (uint16_t)new_brightness);
    brightness_curr_val = new_brightness;
}

void display_brightness_configure() {
    gpio_set_function(TFT_BACKLIT_PIN, GPIO_FUNC_PWM);

    pwm_set_irq1_enabled(TFT_BACKLIT_SLICE, true);
    irq_set_exclusive_handler(TFT_BACKLIT_INT_NUM, &display_brightness_isr);
    irq_set_enabled(TFT_BACKLIT_INT_NUM, true);
    irq_set_priority(TFT_BACKLIT_INT_NUM, TFT_BACKLIT_INT_PRI);

    pwm_set_clkdiv(TFT_BACKLIT_SLICE, 1.f);
    pwm_set_wrap(TFT_BACKLIT_SLICE, 4094);                               // 0 - 4095
    pwm_set_chan_level(TFT_BACKLIT_SLICE, TFT_BACKLIT_PIN & 1, 2048);   // Start at half brightness
    pwm_set_enabled(TFT_BACKLIT_SLICE, true);
}

//// DISPLAY NIGHT MODE ////

/*
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
*/

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