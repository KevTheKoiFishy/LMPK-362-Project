#include    "time_utils.h"

#define     TIME_ZONE_OFFSET    -4
#define     DAYLIGHT_SAVINGS_EN false

void        dst_enable()                    ;
void        dst_disable()                   ;
void        alarm_enable()                  ;
void        alarm_disable()                 ;
void        set_alarm_time (my_alarm_t time);
void        set_time_zone  (int8_t offset)  ;
my_alarm_t  get_alarm_cfg()                 ;

my_time_t   get_utc_time()      ;
my_date_t   get_utc_date()      ;
my_time_t   get_local_std_time();
my_date_t   get_local_std_date();
my_time_t   get_local_dst_time();
my_date_t   get_local_dst_date();
my_time_t   get_local_time()    ;
my_date_t   get_local_date()    ;

void        time_update_isr();
void        alarm_fire_sequence();