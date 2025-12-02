#include "const.h"
#include "ambience_control.h"

uint32_t volume_adc_out = 4095;
float    volume_scalar  = 1.0;

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

uint32_t get_volume_adc() { return volume_adc_out; }    // For debugging
float get_volume_scalar() { return volume_scalar; }     // For debugging

// Clamp between [0, 1]
static float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

// Clamp between [0, 100]
static float clamp_pct(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 100.0f) return 100.0f;
    return x;
}

// Alarm volume ramp-up fxn: Makes the ramp start gentle and increase faster near the end.
static float volume_curve(float x) {
    x = clamp01(x);
    float k = 2.0f;      
    return powf(x, k);    // y = x^k
}

// Execute volume_curve function. Ramps volume between start pct and end pct of the dial setting.
void alarm_volume_ramp_blocking(float start_pct, float end_pct, uint32_t ramp_ms, uint32_t step_ms)
{
    if (step_ms == 0) step_ms = 50;
    if (ramp_ms == 0) ramp_ms = 1;

    start_pct = clamp_pct(start_pct);
    end_pct   = clamp_pct(end_pct);

    uint32_t steps = ramp_ms / step_ms;
    if (steps == 0) steps = 1;

    printf("Starting alarm volume ramp: %.1f%% -> %.1f%% (%ld ms)\n", start_pct, end_pct, ramp_ms);

    for (uint32_t i = 0; i <= steps; i++) {
        float t = (float)i / (float)steps;   // 0 to 1
        float curved = volume_curve(t);     
        float current_pct = start_pct + (end_pct - start_pct) * curved;

        // set volume
        set_audio_volume(current_pct * 0.01f);
        sleep_ms(step_ms);
    }

    printf("Alarm volume ramp done.\n");
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