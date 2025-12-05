#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "const.h"

#include "sd.h"
#include "sd_sdcard.h"
#include "sd_uart.h"
#include "sd_audio.h"

#include "time.h"
#include "gps_uart.h"
#include "gps_time.h"

#include "ambience_control.h"

#include "tft_display.h"
#include "tft_chardisp.h"
#include "tft_main.h"

#include "keypad_queue.h"
#include "keypad.h"

// #define TEST_SD_CMD
// #define TEST_SD_READ_RATE
// #define TEST_SD_AUDIO_PLAYBACK
// #define TEST_GPS_TIME
// #define TEST_AMB_UPDATE
#define TEST_LOCAL_TIME
// #define TEST_KEYPAD
// #define TEST_TFT_MAIN

void core_1_main() {
    core1_loop:
        core1_maintain_audio_buff_routine();
    goto core1_loop;
}

int main() {

    #ifdef TEST_TFT_MAIN
        return tft_main();
    #endif


    // SETUP AND INTERRUPTS HERE
    init_uart();

    printf("\r\n\r\n");
    printf("Booting.");
    
    /////////////////
    // MULTICORE!  //
    /////////////////
    stdio_init_all(); // Enable multicore
    multicore_launch_core1(&core_1_main);
    printf("\n[OK] Core 1: Main Launched");

    /////////////////
    // SD CARD     //
    /////////////////
    init_uart_irq();
    init_sdcard_io();
    printf("\n[OK] SDCard: IO Started");

    /////////////////
    // SD-AUDIO    //
    /////////////////
    mount(0, NULL);
    printf("\n[OK] SDCard: Mounted Volume 0");

    /////////////////
    // VOL, BRIGHT //
    /////////////////
    init_buttons();
    init_buttons_irq();
    printf("\n[OK] Buttons: Int Set");

    init_ambience_adc_and_dma();
    printf("\n[OK] Knob, LDR: ADC, DMA Started");
    
    init_volume_ramp_int();
    printf("\n[OK] Volume Ramp: Int Set");

    display_brightness_configure();
    printf("\n[OK] Display Backlit: Int Set");

    /////////////////
    // KEYPAD      //
    /////////////////
    keypad_init_pins();
    keypad_init_timer();
    printf("\n[OK] Keypad: Pins, Timer started");

    /////////////////
    // GPS-UART    //
    /////////////////
    init_gps_uart(); // GPS time updates automatically!!
    printf("\n[OK] GPS-UART: Sream parse routine started");

    /////////////////
    // TIME        //
    /////////////////
    config_time_update_int();
    printf("\n[OK] Time: Update Int Set");

    /////////////////
    // DISPLAY     //
    /////////////////
    tft_init_pins();
    tft_init();
    cd_clear();
    printf("\n[OK] TFT Display: Initialized");

    #ifdef TEST_SD_CMD
        sd_command_shell(); // uncomment for debug
    #endif
    
    #ifdef TEST_SD_READ_RATE
        open_sd_audio_file(DEFAULT_AUDIO_PATH);
        audio_file_result buff_load_result = 0;
        uint32_t pos_0 = get_data_offset();
        uint32_t tick  = time_us_32();

        uint16_t i = 1000;
        while(!buff_load_result && --i){
            buff_load_result = fill_audio_buffer();
        }

        uint32_t tock     = time_us_32();
        uint32_t pos_f    = get_data_offset();
        float    byterate = 1000000.0 * (pos_f - pos_0) / (tock - tick);
        printf("Average Read Byte Rate: %lu bytes over %lu us = %.2f bytes/sec", pos_f - pos_0, tock - tick, byterate);
    #endif

    #ifdef TEST_SD_AUDIO_PLAYBACK
        open_sd_audio_file(DEFAULT_AUDIO_PATH);
        configure_audio_play();
        start_audio_playback();
    #endif

    // TIME VARIABLES
    my_time_t    utc_time      ;
    my_date_t    utc_date      ;
    my_time_t    local_std_time;
    my_date_t    local_std_date;
    my_time_t    local_dst_time;
    my_date_t    local_dst_date;
    my_time_t    local_time    ;
    my_date_t    local_date    ;
    my_alarm_t   alarm_cfg     ;
    gps_status_t gps_status    ;

    core0_loop:
        utc_time          =   get_utc_time();
        utc_date          =   get_utc_date();
        local_std_time    =   get_local_std_time();
        local_std_date    =   get_local_std_date();
        local_dst_time    =   get_local_dst_time();
        local_dst_date    =   get_local_dst_date();
        local_time        =   get_local_time();
        local_date        =   get_local_date();
        alarm_cfg         =   get_alarm_cfg();
        gps_status        =   gps_get_status();

        alarm_fire_routine();
        cd_update(
            local_time.hours,
            local_time.minutes,
            local_time.seconds,
            gps_status == GOT_DATETIME,
            alarm_cfg.hours,
            alarm_cfg.minutes,
            alarm_cfg.enabled,
            is_audio_playing()
        );
        sleep_ms(1000);

        #ifdef TEST_AMB_UPDATE
            sleep_ms(200);
            printf("\n");
            printf("\nKnob Mode  | %s", get_knob_mode() == BRIGHTNESS_MODE ? "BRIGHTNESS" : "VOLUME");
            printf("\nVolume     | Ramp:  %s, Setting: %4d, Scalar: %6.3f",       get_volume_ramp_en()      ? "T" : "F", get_volume_setting(), get_volume_scalar());
            printf("\nBrightness | Adapt: %s, Setting: %4d, LDR:    %6d, Value: %6d"  ,  get_brightness_adapt_en() ? "T" : "F", get_brightness_setting(), get_brightness_env(), (uint16_t)get_brightness_valf());
        #endif

        #ifdef TEST_GPS_TIME
            sleep_ms(1000);
            printf("\nGPS Time: "); printf(gps_get_timestr()); printf("\n\n");
        #endif

        #ifdef TEST_LOCAL_TIME
            printf("\n--- Local Time Info ---");
            printf("\n  GPS Status     | %s", gps_get_status() == GOT_DATETIME ? "GOT DATETIME" : gps_get_status() == GOT_TIME ? "GOT TIME" : gps_get_status() == CONNECTING ? "CONNECTING" : "UNKNOWN");

            printf("\n  UTC Time       | %02d:%02d:%02d %04d-%02d-%02d", utc_time.hours, utc_time.minutes, utc_time.seconds, utc_date.year, utc_date.month, utc_date.day);
            printf("\n  Local STD Time | %02d:%02d:%02d %04d-%02d-%02d", local_std_time.hours, local_std_time.minutes, local_std_time.seconds, local_std_date.year, local_std_date.month, local_std_date.day);
            printf("\n  Local DST Time | %02d:%02d:%02d %04d-%02d-%02d", local_dst_time.hours, local_dst_time.minutes, local_dst_time.seconds, local_dst_date.year, local_dst_date.month, local_dst_date.day);
            printf("\n  Local Time     | %02d:%02d:%02d %04d-%02d-%02d", local_time.hours, local_time.minutes, local_time.seconds, local_date.year, local_date.month, local_date.day);
            printf("\n  Alarm Config   | %02d:%02d:00, %s %s", alarm_cfg.hours, alarm_cfg.minutes, alarm_cfg.enabled ? "ENABLED" : "DISABLED", alarm_cfg.fire ? "FIRING" : "");
            printf("\n-----------------------\n");
        #endif

        #ifdef TEST_KEYPAD
            uint16_t keyevent = key_pop();

            if (keyevent == (uint16_t)-1) {
                printf("\nKEV: No Key Event\n");
            }
            else {
                if (keyevent & (1 << 8))
                    printf("\nKEV: Pressed  %c\n", (char) (keyevent & 0xFF));
                else if (keyevent != 0)
                    printf("\nKEV: Released %c\n", (char) (keyevent & 0xFF));
            }
        #endif

    goto core0_loop;
    
}