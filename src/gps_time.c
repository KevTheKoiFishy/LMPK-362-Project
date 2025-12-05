#include    <stdint.h>
#include    <stdio.h>

#include    "const.h"
#include    "ministring.h"
#include    "gps_time.h"
#include    "gps_uart.h"
#include    "time_utils.h"

my_time_t    gps_time = {.hours = 0, .minutes = 0, .seconds = 50};
my_date_t    gps_date = {.year = 2000, .month = 1, .day = 1};
my_time_t    gps_get_time() { return gps_time; }
my_date_t    gps_get_date() { return gps_date; }

gps_status_t gps_status = CONNECTING;
// gps_status_t gps_status = GOT_DATETIME;         // FOR TEST
gps_status_t gps_get_status() { return gps_status; }

char         timestr[20];

void gps_parseline(char * buffer, uint16_t length) {
    if (length < 25) { return; }
    // if (length != 25 || length != 37) { return; }            // Is time ready?
    while (*buffer != '$') { ++buffer; }                     // Seek to first $
    while (*(++buffer) == '$');                              // Seek to last $
    if (mini_strcmp(buffer, "GNZDA") < 5) { return; }        // Is this line the time?

    // Get Time
    if (*(buffer + 6) == ',') { return; }                    // Is time ready?
    gps_time.hours   = mini_strtoi(buffer +  6, 2, 10);
    gps_time.minutes = mini_strtoi(buffer +  8, 2, 10);
    gps_time.seconds = mini_strtoi(buffer + 10, 2, 10);
    gps_status       = GOT_TIME;
    if (*(buffer + 17) == ',') { return; }                  // Is date ready?
    gps_date.day     = mini_strtoi(buffer + 17, 2, 10);
    gps_date.month   = mini_strtoi(buffer + 20, 2, 10);
    gps_date.year    = mini_strtoi(buffer + 23, 4, 10);
    gps_status       = GOT_DATETIME;

    // Disable interrupt after getting both, and only update time locally. Re-enable interrupt each hour to sync.
    disable_gps_uart();
}

char * gps_get_timestr() {
    sprintf(timestr, "%02d:%02d:%02d %02d-%02d-%04d", gps_time.hours, gps_time.minutes, gps_time.seconds, gps_date.month, gps_date.day, gps_date.year);
    return timestr;
}