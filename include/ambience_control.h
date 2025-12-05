#ifndef AMBIENCE_CONTROL_H
#define AMBIENCE_CONTROL_H

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"

// Knob Controls what?
typedef enum {
    BRIGHTNESS_MODE = 0,
    VOLUME_MODE     = 1
} knobMode;

// Night mode logic
typedef struct {
    int     night_start_hour;     // military time (ex. 23 == 11 pm)
    int     night_end_hour;        
    float   day_max_brightness;   // 0 to 100
    float   night_max_brightness;  
} NightModeConfig;

// Buttons
void init_buttons(void);
void buttons_isr();
void init_buttons_irq();

// Short Functions
knobMode    get_knob_mode();

uint16_t    get_volume_setting() ;
float       get_volume_scalar()  ;
bool        get_volume_ramp_en() ;

uint16_t    get_brightness_setting()  ;
uint16_t    get_brightness_env()      ;
float       get_brightness_valf()     ;
bool        get_brightness_adapt_en() ;

// ADC
#define     ADC_ROUNDROBIN_BUF_LEN_LSB  1 // Len must be power of 2 for DMA's sake
#define     ADC_ROUNDROBIN_BUF_LEN      (1 << ADC_ROUNDROBIN_BUF_LEN_LSB)

void        init_ambience_adc_and_dma();

// Volume Ramping
#define     VOLUME_RAMP_DEFAULT_STEP_MS 50
#define     VOLUME_RAMP_DEFAULT_RAMP_MS 7000
#define     VOLUME_RAMP_POW_CURVE_EXP   2.0f

void        alarm_volume_ramp_blocking(float start_pct, float end_pct, uint16_t ramp_ms, uint16_t step_ms);
// bool        volume_ramp_isr(__unused struct repeating_timer *t);
void        volume_ramp_isr();
void        configure_volume_ramp_int(float start_ratio, float end_ratio, uint16_t ramp_ms, uint16_t step_ms);
void        init_volume_ramp_int();
void        enable_volume_ramp_int();
void        disable_volume_ramp_int();

// Adaptive Brightness
void        display_brightness_isr();
void        display_brightness_configure();

// float       apply_night_mode_brightness(float knob_pct, int current_hour_24, const NightModeConfig *cfg);

// __weak void set_brightness_percent(float b) { float brightness_pct = b; }

// void        control_update(void);

#endif