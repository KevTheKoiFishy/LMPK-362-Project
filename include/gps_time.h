
typedef struct {
    uint8_t  hh;
    uint8_t  mm;
    uint8_t  ss;
    uint8_t  DD;
    uint8_t  MM;
    uint16_t YYYY;
} utc_time_t;

void gps_parseline(char * buffer, uint16_t length);
char * gps_get_timestr();