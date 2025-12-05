#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <stdint.h>
#include <stdbool.h>

// Time structure (24-hour format)
typedef struct {
    uint8_t hours;      // 0-23
    uint8_t minutes;    // 0-59
    uint8_t seconds;    // 0-59
} my_time_t;

// Time structure (12-hour format)
typedef struct {
    uint8_t hours;      // 1-12
    uint8_t minutes;    // 0-59
    uint8_t seconds;    // 0-59
    bool is_pm;         // true = PM, false = AM
} time_12h_t;

// Date structure
typedef struct {
    uint16_t year;      // e.g., 2025
    uint8_t month;      // 1-12
    uint8_t day;        // 1-31
} my_date_t;

// Alarm structure
typedef struct {
    uint8_t hours;      // 0-23
    uint8_t minutes;    // 0-59
    bool    enabled;    // true = alarm on, false = alarm off
    bool    fire;       // true = fired this second
} my_alarm_t;

// Stopwatch structure
typedef struct {
    uint32_t milliseconds;  // Total elapsed time
} my_stopwatch_t;

// Timer structure
typedef struct {
    uint32_t remaining_ms;  // Remaining time in milliseconds
    bool running;           // true = counting down, false = stopped
} my_timer_t;

// Function prototypes
my_date_t   convert_timezone_date(my_time_t utc_time, my_date_t utc_date, int8_t offset_hours);
my_time_t   convert_timezone(my_time_t utc_time, int8_t offset_hours);

time_12h_t  convert_to_12h(my_time_t time_24h);
bool        is_leap_year(uint16_t year);
uint8_t     days_in_month(uint8_t month, uint16_t year);

my_date_t   increment_date(my_date_t current);
my_date_t   decrement_date(my_date_t current);

int8_t      daylight_savings_US(my_date_t standard_tz_date, my_time_t standard_tz_time);

bool        check_alarm(my_time_t current, my_alarm_t alarm);

void        increment_second_inplace(my_date_t * date, my_time_t * time);
void        stopwatch_to_display(my_stopwatch_t sw, uint8_t* minutes, uint8_t* seconds, uint8_t* centiseconds);
bool        timer_expired(my_timer_t* t);
void        timer_tick(my_timer_t* t, uint32_t elapsed_ms);
void        test_time_functions(void);

#endif // TIME_UTILS_H
