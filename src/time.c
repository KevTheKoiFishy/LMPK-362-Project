#include    "time.h"
#include    "time_utils.h"

#include    "gps_time.h"

#include    "sd_audio.h"

my_time_t   utc_time, local_std_time, local_dst_time;
my_date_t   utc_date, local_std_date, local_dst_date;
my_alarm_t  alarm_cfg;

bool        use_daylight_savings                = true;
bool        time_zone_offset                    = TIME_ZONE_OFFSET; // STANDARD time zone offset

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

void time_update_isr() {
    static bool datetime_obtained = false;

    // TODO: ACK

    if (gps_get_status() == CONNECTING) { return; }

    if (!datetime_obtained){
        // Get UTC Time
        utc_time        = gps_get_time();
        utc_date        = gps_get_date();
        if (gps_get_status() == GOT_DATETIME) { datetime_obtained = true; }

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
}

//// ALARM ////

void alarm_fire_sequence() {
    if (!alarm_cfg.fire) { return; }
    alarm_cfg.fire = false;

    audio_play_sequence("");
}

//// STOPWATCH ////

//// TIMER ////

