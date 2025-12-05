#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "tft_chardisp.h"
#include "tft_display.h"

int tft_main()
{
    tft_init_pins();

    tft_init();
    cd_clear();

    // Current time
    int hour = 5;
    int minute = 59;
    int second = 50;

    // Alarm time
    int alarm_hour = 6;
    int alarm_minute = 0;
    bool alarm_enabled = true;

    // Is the current time accurate
    bool gps_found = false;
    int  gps_counter = 0;

    // Is the alarm ringing
    bool alarm_ringing = false;
    int  alarm_ring_time = 0;

    while (true) {
        cd_update(hour, minute, second, gps_found, alarm_hour, alarm_minute, alarm_enabled, alarm_ringing);
        sleep_ms(1000);

        // Increase time
        second++;
        if (second >= 60) 
        {
            second = 0;
            minute++;
        }
        if (minute >= 60) 
        {
            minute = 0;
            hour++;
        }
        if (hour >= 24) 
        {
            hour = 0;
        }

        // Assume current time is accurate
        if (!gps_found) 
        {
            gps_counter++;
            if (gps_counter >= 5) 
            {
                gps_found = true;
            }
        }

        // Start alarm
        if (!alarm_ringing && alarm_enabled && hour == alarm_hour && minute == alarm_minute && second == 0) 
        {
            alarm_ringing = true;
            alarm_ring_time = 0;
        }

        // Stop alarm after 20 seconds
        if (alarm_ringing) 
        {
            alarm_ring_time++;
            if (alarm_ring_time >= 20) 
            {
                alarm_ringing = false;
                alarm_enabled = false;
            }
        }
    }

    return 0;
}