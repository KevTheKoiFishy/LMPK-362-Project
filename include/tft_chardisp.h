#ifndef TFT_CHARDISP_H
#define TFT_CHARDISP_H

#include <stdbool.h>
#include <stdint.h>

// Current time
#define CURR_NUM_W         24   // Width of current time number
#define CURR_NUM_H         40   // Height of current time number
#define NUM_THICK          4    // Number thickness
#define NUM_SPACING        6    // Space between numbers
#define CURR_COLON         8    // Width of colon
#define CURR_POS           20   // Vertical position of current time

// Alarm time positon
#define ALARM_POS          (CURR_POS + CURR_NUM_H + 12)

// Status text at bottom
#define STATUS_SCALE           2
#define STATUS_CHAR_W          (TEXT_W * STATUS_SCALE)
#define STATUS_CHAR_H          (TEXT_H * STATUS_SCALE)
#define STATUS_LINE_SPACING    6
// Position text from bottom up
#define STATUS_BOTTOM_MARGIN   12
#define STATUS_Y_BOTTOM        (TFT_HEIGHT - STATUS_CHAR_H - STATUS_BOTTOM_MARGIN)
#define STATUS_Y_TOP           (STATUS_Y_BOTTOM - STATUS_CHAR_H - STATUS_LINE_SPACING)
#define STATUS_MAX_CHARS       16

// Bell position
#define BELL_W      80
#define BELL_H      80
#define BELL_POS    180   // Vertical position

void cd_clear();
void cd_alarm(int hour, int minute, bool enabled);
void cd_status(const char *status);
void current_time(int hour, int minute);
void draw_status(bool gps_found, bool alarm_enabled, bool alarm_ringing);
void clear_bell();
void draw_bell(bool tilt);
void cd_update(int hour, int minute, int second, bool gps_found, int alarm_hour, int alarm_minute, bool alarm_enabled, bool alarm_ringing);

#endif