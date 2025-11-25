#include <stdint.h>
#include <stdio.h>
#include "ministring.h"
#include "gps_time.h"

utc_time_t  gps_time = { .hh = 0, .mm = 0, .ss = 0, .DD = 0, .MM = 0, .YYYY = 0 };
char        timestr[20];

void gps_parseline(char * buffer, uint16_t length) {
    if (length < 25) { return; }
    // if (length != 25 || length != 37) { return; }            // Is time ready?
    while (*buffer != '$') { ++buffer; }                     // Seek to first $
    while (*(++buffer) == '$');                              // Seek to last $
    if (mini_strcmp(buffer, "GNZDA") < 5) { return; }        // Is this line the time?

    // Get Time
    if (*(buffer + 6) == ',') { return; }                    // Is time ready?
    gps_time.hh   = mini_strtoi(buffer +  6, 2, 10);
    gps_time.mm   = mini_strtoi(buffer +  8, 2, 10);
    gps_time.ss   = mini_strtoi(buffer + 10, 2, 10);
    if (*(buffer + 17) == ',') { return; }                   // Is date ready?
    gps_time.DD   = mini_strtoi(buffer + 17, 2, 10);
    gps_time.MM   = mini_strtoi(buffer + 20, 2, 10);
    gps_time.YYYY = mini_strtoi(buffer + 23, 4, 10);
}

char * gps_get_timestr() {
    timestr[sprintf(timestr, "%02d:%02d:%02d %02d-%02d-%04d", gps_time.hh, gps_time.mm, gps_time.ss, gps_time.DD, gps_time.MM, gps_time.YYYY)] = 0;
    return timestr;
}