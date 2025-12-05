#include "time_utils.h"

typedef enum {
    CONNECTING   = 1,
    GOT_TIME     = 2,
    GOT_DATETIME = 3
} gps_status_t;

my_time_t gps_get_time();
my_date_t gps_get_date();
gps_status_t gps_get_status();

void    gps_parseline(char * buffer, uint16_t length);
char *  gps_get_timestr();