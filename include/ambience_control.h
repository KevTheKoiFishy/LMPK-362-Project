#ifndef AMBIENCE_CONTROL_H
#define AMBIENCE_CONTROL_H

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

// Night mode logic
typedef struct {
    int   night_start_hour;     // military time (ex. 23 == 11 pm)
    int   night_end_hour;        
    float day_max_brightness;   // 0 to 100
    float night_max_brightness;  
} NightModeConfig;

#define  set_audio_volume(v) volume_scalar = v;

void init_volume_adc_freerun();
void init_volume_dma();
void init_volume_adc_and_dma();

uint32_t get_volume_adc();
float get_volume_scalar();
void alarm_volume_ramp_blocking(float start_pct, float end_pct, uint32_t ramp_ms, uint32_t step_ms);

float apply_night_mode_brightness(float knob_pct, int current_hour_24, const NightModeConfig *cfg);

__weak void set_brightness_percent(float b);

#endif