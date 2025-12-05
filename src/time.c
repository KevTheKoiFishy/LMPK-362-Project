#include    <hardware/timer.h>
#include    <hardware/irq.h>

#include    "arbitration.h"

#include    "time.h"
#include    "time_utils.h"

#include    "gps_time.h"

#include    "sd_audio.h"

my_time_t   utc_time, local_std_time, local_dst_time;
my_date_t   utc_date, local_std_date, local_dst_date;
my_alarm_t  alarm_cfg = {.hours = 20, .minutes = 1, .enabled = true, .fire = false};

bool        use_daylight_savings                = false;
int8_t      time_zone_offset                    = TIME_ZONE_OFFSET; // STANDARD time zone offset

void        dst_enable()                        { use_daylight_savings = true;  }
void        dst_disable()                       { use_daylight_savings = false; }
void        alarm_enable()                      { alarm_cfg.enabled = true;     }
void        alarm_disable()                     { alarm_cfg.enabled = false;    }
void        set_alarm_time (my_alarm_t time)    { alarm_cfg = time;             }
void        set_time_zone  (int8_t offset)      { time_zone_offset = offset;    }
my_alarm_t  get_alarm_cfg()                     { return alarm_cfg;             }

my_time_t   get_utc_time()                      { return utc_time;              }
my_date_t   get_utc_date()                      { return utc_date;              }
my_time_t   get_local_std_time()                { return local_std_time;        }
my_date_t   get_local_std_date()                { return local_std_date;        }
my_time_t   get_local_dst_time()                { return local_dst_time;        }
my_date_t   get_local_dst_date()                { return local_dst_date;        }
my_time_t   get_local_time()                    { return use_daylight_savings ? local_dst_time : local_std_time; }
my_date_t   get_local_date()                    { return use_daylight_savings ? local_dst_date : local_std_date; }

//// TIME UPDATES ////

// Update time each second
// bool time_update_isr(__unused struct repeating_timer *t) {
void time_update_isr() {
    static bool datetime_obtained = false;

    // Acknowledge
    TIME_STEP_TIMER_HW -> intr |= 1 << TIME_STEP_ALARM;

    if (gps_get_status() == CONNECTING) { return; }

    if (!datetime_obtained){
        if (gps_get_status() == GOT_DATETIME) { datetime_obtained = true; }
        // Get UTC Time
        utc_time        = gps_get_time();
        utc_date        = gps_get_date();

        // Get Standard Time
        local_std_time  = convert_timezone(utc_time, time_zone_offset);
        local_std_date  = convert_timezone_date(utc_time, utc_date, time_zone_offset);
    } else {
        increment_second_inplace(&utc_date,       &utc_time);
        increment_second_inplace(&local_std_date, &local_std_time);
    }

    // Get dst time
    if (use_daylight_savings) {
        int8_t dst_offset = daylight_savings_US(local_std_date, local_std_time);

        local_dst_time  = convert_timezone(local_std_time, dst_offset);
        local_dst_date  = convert_timezone_date(local_std_time, local_std_date, dst_offset);
    }

    // Check for alarm
    alarm_cfg.fire      = check_alarm(get_local_time(), alarm_cfg);

    // Update next isr time.
    TIME_STEP_TIMER_HW -> alarm[TIME_STEP_ALARM] += 1000000; // 1 second
}

// Configure time update interrupt
void config_time_update_int() {
    // Set up initial trigger time
    TIME_STEP_TIMER_HW -> inte |= (1 << TIME_STEP_ALARM);
    
    // Enable interrupt
    irq_handler_t current_handler = irq_get_exclusive_handler(TIME_STEP_INT_NUM);
    // irq_set_exclusive_handler(TIME_STEP_INT_NUM, NULL);
    irq_set_exclusive_handler(TIME_STEP_INT_NUM, &time_update_isr);
    // irq_add_shared_handler(TIME_STEP_INT_NUM, &timer0_alarm_irq_dispatcher, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    irq_set_priority(TIME_STEP_INT_NUM, TIME_STEP_INT_PRI);
    irq_set_enabled(TIME_STEP_INT_NUM, true);
    TIME_STEP_TIMER_HW -> alarm[TIME_STEP_ALARM] = TIME_STEP_TIMER_HW -> timerawl + 1000000; // 1 second

    // timer_hardware_alarm_set_callback(TIME_STEP_TIMER_HW, TIME_STEP_ALARM, &timer0_alarm_irq_dispatcher);
    // add_repeating_timer_ms(-1000, &time_update_isr, NULL, NULL); // alternative to above line
}

//// ALARM ////

// Trigger alarm sequence if alarm fired
void alarm_fire_routine() {
    if (!alarm_cfg.fire) { return; }
    alarm_cfg.fire = false;

    audio_play_sequence("");
}

//// STOPWATCH ////

//// TIMER ////

