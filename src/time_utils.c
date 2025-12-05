#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "time_utils.h"

// Convert UTC to local date
my_date_t convert_timezone_date(my_time_t utc_time, my_date_t utc_date, int8_t offset_hours) {
    if ((int8_t)utc_time.hours + offset_hours < 0)  { return decrement_date(utc_date); }
    if ((int8_t)utc_time.hours + offset_hours > 23) { return increment_date(utc_date); }
    return utc_date;
}

// Convert UTC to local time (e.g., EST is UTC-5, EDT is UTC-4)
my_time_t convert_timezone(my_time_t utc_time, int8_t offset_hours) {
    my_time_t local = utc_time;
    local.hours = (utc_time.hours + offset_hours + 24) % 24;
    return local;
}

// Convert 24-hour format to 12-hour format
time_12h_t convert_to_12h(my_time_t time_24h) {
      time_12h_t result;
      result.minutes = time_24h.minutes;
      result.seconds = time_24h.seconds;

      if (time_24h.hours == 0) {
          result.hours = 12;
          result.is_pm = false;  // midnight
      } else if (time_24h.hours < 12) {
          result.hours = time_24h.hours;
          result.is_pm = false;
      } else if (time_24h.hours == 12) {
          result.hours = 12;
          result.is_pm = true;   // noon
      } else {
          result.hours = time_24h.hours - 12;
          result.is_pm = true;
      }

    return result;
}

// Check if year is leap year
bool is_leap_year(uint16_t year) {
    if (year % 400 == 0) return true;
    if (year % 100 == 0) return false;
    if (year % 4 == 0) return true;
    return false;
}

// Get day of week
uint8_t day_of_week(my_date_t date) {
    uint16_t d = date.day, m = date.month, y = date.year;
    return (d += m < 3 ? y-- : y - 2, 23*m/9 + d + 4 + y/4- y/100 + y/400)%7;  
}

// Get which week of the month it is
uint8_t week_of_month(my_date_t date) {
    uint8_t DOW_this_day    = day_of_week(date);
    uint8_t DOW_day_1       = (DOW_this_day + 36 - date.day) % 7;
    uint8_t week_in_month   = ((date.day - 1) + DOW_day_1) / 7;
    return week_in_month;
}

// Get days in month
uint8_t days_in_month(uint8_t month, uint16_t year) {
    uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month == 2 && is_leap_year(year)) {
        return 29;
    }

    return days[month - 1];
}

// Increment date by one day
my_date_t increment_date(my_date_t current) {
    my_date_t next = current;
    next.day++;

    if (next.day > days_in_month(next.month, next.year)) {
        next.day = 1;
        next.month++;

        if (next.month > 12) {
            next.month = 1;
            next.year++;
        }
    }

    return next;
}

// Decrement date by one day
my_date_t decrement_date(my_date_t current) {
    my_date_t prev = current;

    if (--prev.day == 0) {
        if (--prev.month == 0) {
            prev.month = 12;
            --prev.year;
        }
        prev.day = days_in_month(prev.month, prev.year);
    }

    return prev;
}

// Obtain offset from standard time according to US daylight savings rules.
int8_t daylight_savings_US(my_date_t standard_tz_date, my_time_t standard_tz_time) {

    if (3 < standard_tz_date.month && standard_tz_date.month < 11) { return 1; }

    // Move forward
    if (standard_tz_date.month == 3) {
        uint8_t DOW             = day_of_week(standard_tz_date);
        uint8_t sundays_elapsed = week_of_month(standard_tz_date) + ((DOW + 36 - standard_tz_date.day) % 7 == 0);
        if (sundays_elapsed < 2) { return 0; }
        if (sundays_elapsed > 2) { return 1; }
        if (DOW == 0) {
            if (standard_tz_time.hours >= 2) { return 1; }
            return 0;
        }
        return 1;
    }

    // Fall back
    if (standard_tz_date.month == 11) {
        uint8_t DOW             = day_of_week(standard_tz_date);
        uint8_t sundays_elapsed = week_of_month(standard_tz_date) + ((DOW + 36 - standard_tz_date.day) % 7 == 0);
        if (sundays_elapsed < 1) { return 1; }
        if (sundays_elapsed > 1) { return 0; }
        if (DOW == 0) {
            if (standard_tz_time.hours >= 1) { return 0; }
            return 1;
        }
        return 0;
    }

    // All other cases
    return 0;
}

// Increment in place
void increment_second_inplace(my_date_t * date, my_time_t * time) {
    time -> seconds += 1;
    if (time -> seconds >= 60) { ++time -> minutes; time -> seconds = 0; } else { return; }
    if (time -> minutes >= 60) { ++time -> hours;   time -> minutes = 0; } else { return; }
    if (time -> hours   >= 24) { ++date -> day;     time -> hours   = 0; } else { return; }
    
    uint8_t MD = days_in_month(date -> month, date -> year);
    if (date -> day     >  MD) { ++date -> month;   date -> day     = 1; } else { return; }
    if (date -> month   >  12) { ++date -> year;    date -> month   = 1; } else { return; }
}

// Check if current time matches alarm
bool check_alarm(my_time_t current, my_alarm_t alarm) {
      if (!alarm.enabled) {
        return false;
      } 

    return (current.hours == alarm.hours &&
            current.minutes == alarm.minutes &&
            current.seconds == 0);  // Trigger on :00 seconds
}

// Convert milliseconds to displayable format
void stopwatch_to_display(my_stopwatch_t sw, uint8_t* minutes,
                          uint8_t* seconds, uint8_t* centiseconds) {
      uint32_t total_seconds = sw.milliseconds / 1000;
      *minutes = total_seconds / 60;
      *seconds = total_seconds % 60;
    *centiseconds = (sw.milliseconds % 1000) / 10;
}

bool timer_expired(my_timer_t* t) {
    return (t->running && t->remaining_ms == 0);
}

void timer_tick(my_timer_t* t, uint32_t elapsed_ms) {
    if (t->running && t->remaining_ms > 0) {
        if (elapsed_ms >= t->remaining_ms) {
            t->remaining_ms = 0;
        } else {
            t->remaining_ms -= elapsed_ms;
        }
    }
}

// Test function - demonstrates all time utilities
void test_time_functions(void) {
    printf("\n=== Time Utility Functions Test ===\n\n");

    // Test 1: Timezone conversion
    printf("1. Time Zone Conversion\n");
    my_time_t utc = {.hours = 18, .minutes = 45, .seconds = 30};
    my_time_t est = convert_timezone(utc, -5);
    my_time_t pst = convert_timezone(utc, -8);
    printf("   UTC: %02d:%02d:%02d\n", utc.hours, utc.minutes, utc.seconds);
    printf("   EST: %02d:%02d:%02d (UTC-5)\n", est.hours, est.minutes, est.seconds);
    printf("   PST: %02d:%02d:%02d (UTC-8)\n\n", pst.hours, pst.minutes, pst.seconds);

    // Test 2: 12-hour format
    printf("2. 12-Hour Format Conversion\n");
    time_12h_t display = convert_to_12h(est);
    printf("   24h: %02d:%02d -> 12h: %02d:%02d %s\n\n",
           est.hours, est.minutes, display.hours, display.minutes,
           display.is_pm ? "PM" : "AM");

    // Test 3: Leap year
    printf("3. Leap Year Check\n");
    printf("   2024 is leap year: %s\n", is_leap_year(2024) ? "YES" : "NO");
    printf("   2025 is leap year: %s\n", is_leap_year(2025) ? "YES" : "NO");
    printf("   2000 is leap year: %s\n\n", is_leap_year(2000) ? "YES" : "NO");

    // Test 4: Days in month
    printf("4. Days in Month\n");
    printf("   February 2024: %d days\n", days_in_month(2, 2024));
    printf("   February 2025: %d days\n", days_in_month(2, 2025));
    printf("   April 2025: %d days\n\n", days_in_month(4, 2025));

    // Test 5: Date increment
    printf("5. Date Increment\n");
    my_date_t d1 = {.year = 2025, .month = 2, .day = 28};
    my_date_t d2 = increment_date(d1);
    printf("   %04d-%02d-%02d + 1 day = %04d-%02d-%02d\n",
           d1.year, d1.month, d1.day, d2.year, d2.month, d2.day);

    my_date_t d3 = {.year = 2025, .month = 12, .day = 31};
    my_date_t d4 = increment_date(d3);
    printf("   %04d-%02d-%02d + 1 day = %04d-%02d-%02d\n\n",
           d3.year, d3.month, d3.day, d4.year, d4.month, d4.day);

    // Test 6: Alarm check
    printf("6. Alarm Check\n");
    my_alarm_t alarm = {.hours = 7, .minutes = 30, .enabled = true};
    my_time_t wake_time = {.hours = 7, .minutes = 30, .seconds = 0};
    my_time_t not_wake = {.hours = 7, .minutes = 31, .seconds = 0};
    printf("   Alarm set for: %02d:%02d\n", alarm.hours, alarm.minutes);
    printf("   Time %02d:%02d:%02d triggers: %s\n",
           wake_time.hours, wake_time.minutes, wake_time.seconds,
           check_alarm(wake_time, alarm) ? "YES" : "NO");
    printf("   Time %02d:%02d:%02d triggers: %s\n\n",
           not_wake.hours, not_wake.minutes, not_wake.seconds,
           check_alarm(not_wake, alarm) ? "YES" : "NO");

    // Test 7: Stopwatch
    printf("7. Stopwatch Display\n");
    my_stopwatch_t sw = {.milliseconds = 125678};  // 2 min, 5.67 sec
    uint8_t min, sec, csec;
    stopwatch_to_display(sw, &min, &sec, &csec);
    printf("   %lu ms = %02d:%02d.%02d\n\n", (unsigned long)sw.milliseconds, min, sec, csec);

    // Test 8: Timer
    printf("8. Timer Countdown\n");
    my_timer_t timer = {.remaining_ms = 5000, .running = true};
    printf("   Initial: %lu ms remaining\n", (unsigned long)timer.remaining_ms);
    timer_tick(&timer, 2000);
    printf("   After 2s: %lu ms remaining\n", (unsigned long)timer.remaining_ms);
    timer_tick(&timer, 4000);
    printf("   After 4s more: %lu ms remaining\n", (unsigned long)timer.remaining_ms);
    printf("   Timer expired: %s\n\n", timer_expired(&timer) ? "YES" : "NO");

    printf("=== All Tests Complete! ===\n\n");
}