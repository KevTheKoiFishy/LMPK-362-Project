#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "tft_display.h"
#include "tft_chardisp.h"

// Clear screen
void cd_clear()
{
    fill_screen(TFT_BLACK);
}

// 7-segment display:
//   ---a---
//  |       |
//  f       b
//  |       |
//   ---g---
//  |       |
//  e       c
//  |       |
//   ---d---
static const uint8_t curr_num_segments[10] = {
    0x3F, // 0: a b c d e f
    0x06, // 1:   b c
    0x5B, // 2: a b   d e   g
    0x4F, // 3: a b c d     g
    0x66, // 4:   b c     f g
    0x6D, // 5: a   c d   f g
    0x7D, // 6: a   c d e f g
    0x07, // 7: a b c
    0x7F, // 8: a b c d e f g
    0x6F  // 9: a b c d   f g
};

static void draw_num_segment(int x, int y, int w, int h, uint16_t color)
{
    fill_rect(x, y, w, h, color);
}

// Helper: draw one segment if its bit is set in 'seg'
static void draw_curr_num_segment(uint8_t seg, uint8_t onmask, int x, int y, int w, int h, uint16_t fg, uint16_t bg)
{
    uint16_t color = (seg & onmask) ? fg : bg;
    draw_num_segment(x, y, w, h, color);
}

// Draw each number of current time at (x, y)
static void draw_curr_num(int x, int y, int num, uint16_t fg, uint16_t bg)
{
    if (num < 0 || num > 9) return;

    int w = CURR_NUM_W;
    int h = CURR_NUM_H;
    int thick = NUM_THICK;
    int half = h / 2;

    uint8_t seg = curr_num_segments[num];

    // Segment a
    draw_curr_num_segment(seg, 0x01, x + thick, y, w - 2 * thick, thick, fg, bg);

    // Segment b
    draw_curr_num_segment(seg, 0x02, x + w - thick, y + thick, thick, half - thick - thick / 2, fg, bg);

    // Segment c
    draw_curr_num_segment(seg, 0x04, x + w - thick, y + half + thick / 2, thick, half - thick - thick / 2, fg, bg);

    // Segment d
    draw_curr_num_segment(seg, 0x08, x + thick, y + h - thick, w - 2 * thick, thick, fg, bg);

    // Segment e
    draw_curr_num_segment(seg, 0x10, x, y + half + thick / 2, thick, half - thick - thick / 2, fg, bg);

    // Segment f
    draw_curr_num_segment(seg, 0x20, x, y + thick, thick, half - thick - thick / 2, fg, bg);

    // Segment g
    draw_curr_num_segment(seg, 0x40, x + thick, y + half - thick / 2, w - 2 * thick, thick, fg, bg);
}

// Colon between hour and minute
static void draw_colon(int x, int y, uint16_t fg, uint16_t bg)
{
    int dot_w = 4;
    int dot_h = 6;

    int y_top = y + CURR_NUM_H / 3;
    int y_bottom = y + (2 * CURR_NUM_H) / 3;

    fill_rect(x, y_top, dot_w, dot_h, fg);
    fill_rect(x, y_bottom, dot_w, dot_h, fg);
}

void current_time(int hour, int minute)
{
    if (hour < 0) hour = 0;
    if (hour > 23) hour = 23;
    if (minute < 0) minute = 0;
    if (minute > 59) minute = 59;

    int h_tens = hour / 10;
    int h_ones = hour % 10;
    int m_tens = minute / 10;
    int m_ones = minute % 10;

    int digits_w = 4 * CURR_NUM_W;
    int spaces_w = 3 * NUM_SPACING;
    int total_w = digits_w + spaces_w + CURR_COLON;

    int start_x = (TFT_WIDTH - total_w) / 2;
    int y = CURR_POS;

    uint16_t fg = TFT_WHITE;
    uint16_t bg = TFT_BLACK;

    int x = start_x;

    // Hour tens
    draw_curr_num(x, y, h_tens, fg, bg);
    x += CURR_NUM_W + NUM_SPACING;

    // Hour ones
    draw_curr_num(x, y, h_ones, fg, bg);
    x += CURR_NUM_W + NUM_SPACING;

    // Colon
    draw_colon(x, y, fg, bg);
    x += CURR_COLON + NUM_SPACING;

    // Minute tens
    draw_curr_num(x, y, m_tens, fg, bg);
    x += CURR_NUM_W + NUM_SPACING;

    // Minute ones
    draw_curr_num(x, y, m_ones, fg, bg);
}

// Alarm time
void cd_alarm(int hour, int minute, bool enabled)
{
    static int prev_hour = -1;
    static int prev_minute = -1;
    static bool prev_enabled = false;

    // Only redraw if something changed
    if (hour == prev_hour && minute == prev_minute && enabled == prev_enabled) 
    {
        return;
    }

    prev_hour = hour;
    prev_minute = minute;
    prev_enabled = enabled;

    // Print alarm time
    char buf[20];
    snprintf(buf, sizeof(buf), "Alarm: %02d:%02d", hour, minute);

    int scale = 2;
    int text_length = strlen(buf);
    int text_width = text_length * TEXT_W * scale;
    int x = (TFT_WIDTH - text_width) / 2;
    int y = ALARM_POS;
    int h = TEXT_H * scale + 4;

    fill_rect(0, y - 2, TFT_WIDTH, h, TFT_BLACK);
    draw_string_scaled(x, y, buf, scale, TFT_YELLOW, TFT_BLACK);
}

// Status text
void cd_status(const char *status)
{
    // Clear space for status
    fill_rect(0, STATUS_Y_TOP, TFT_WIDTH, STATUS_CHAR_H * 2 + STATUS_LINE_SPACING, TFT_BLACK);

    if (status != NULL && status[0] != '\0') 
    {
        int text_width = strlen(status) * (TEXT_W * STATUS_SCALE);
        int x = (TFT_WIDTH - text_width) / 2;
        draw_string_scaled(x, STATUS_Y_TOP, status, STATUS_SCALE, TFT_BLUE, TFT_BLACK);
    }
}

// Centers text
static void center_text(int y, const char *text, int max_chars, uint16_t fg, uint16_t bg)
{
    int length = strlen(text);
    if (length > max_chars) length = max_chars;
    
    int text_width = length * STATUS_CHAR_W;
    int x = (TFT_WIDTH - text_width) / 2;
    
    for (int i = 0; i < length; i++) {
        int cx = x + i * STATUS_CHAR_W;
        draw_char_scaled(cx, y, text[i], STATUS_SCALE, fg, bg);
    }
}

// Draws status text at the bottom
void draw_status(bool gps_found, bool alarm_enabled, bool alarm_ringing)
{
    static bool prev_gps_found = false;
    static bool prev_alarm_enabled = false;
    static bool prev_alarm_ringing = false;
    static bool first = true;

    // If nothing changed since last frame, do nothing
    if (!first && gps_found == prev_gps_found && alarm_enabled == prev_alarm_enabled && alarm_ringing == prev_alarm_ringing) {
        return;
    }

    first = false;
    prev_gps_found = gps_found;
    prev_alarm_enabled = alarm_enabled;
    prev_alarm_ringing = alarm_ringing;

    uint16_t bg = TFT_BLACK;

    // Clear the entire status space when text changes
    int clear_height = STATUS_CHAR_H * 2 + STATUS_LINE_SPACING + STATUS_BOTTOM_MARGIN + 4;
    fill_rect(0, STATUS_Y_TOP, TFT_WIDTH, clear_height, bg);

    // GPS status
    const char *gps_text;
    if (!gps_found) {
        gps_text = "GPS SEARCHING";
    } else {
        gps_text = "GPS FOUND";
    }
    center_text(STATUS_Y_TOP, gps_text, STATUS_MAX_CHARS, TFT_BLUE, bg);

    // Alarm status
    const char *alarm_text;
    if (alarm_ringing) {
        alarm_text = "ALARM RINGING";
    } else if (alarm_enabled) {
        alarm_text = "ALARM ON";
    } else {
        alarm_text = "ALARM OFF";
    }
    center_text(STATUS_Y_BOTTOM, alarm_text, STATUS_MAX_CHARS, TFT_YELLOW, bg);
}

void clear_bell()
{
    // Padding
    int side_pad = 12;
    int bottom_pad = BELL_H / 6 + 4;

    int clear_w = BELL_W + 2 * side_pad;
    int clear_h = BELL_H + side_pad + bottom_pad;

    int x_center = TFT_WIDTH / 2;
    int y_center = BELL_POS;

    int x = x_center - clear_w / 2;
    int y = y_center - BELL_H / 2 - side_pad;  // Start above bell

    fill_rect(x, y, clear_w, clear_h, TFT_BLACK);
}

void draw_bell(bool tilt)
{
    // Clear previous bell if needed
    clear_bell();

    uint16_t color = TFT_YELLOW;

    int x_center = TFT_WIDTH / 2;
    int y_center = BELL_POS;

    // Top left of bell
    int x0 = x_center - BELL_W / 2;
    int y0 = y_center - BELL_H / 2;

    // Tilt
    int dx = (tilt ? -1 : 1) * (BELL_W / 10);
    x0 += dx;

    // Top
    fill_rect(x0 + BELL_W * 1/4, y0, BELL_W * 1/2, BELL_H * 3/20, color);

    // Middle
    fill_rect(x0 + BELL_W * 1/8, y0 + BELL_H * 3/20, BELL_W * 3/4, BELL_H * 1/2, color);

    // Bottom
    fill_rect(x0, y0 + BELL_H * 7/10, BELL_W, BELL_H * 1/6, color);

    // Longer, thinner bottom part
    fill_rect(x_center - (BELL_W / 12), y0 + BELL_H, (BELL_W / 6), (BELL_H / 6), color);
}

// Update display
void cd_update(int hour, int minute, int second, bool gps_found, int alarm_hour, int alarm_minute, bool alarm_enabled, bool alarm_ringing)
{
    current_time(hour, minute);
    cd_alarm(alarm_hour, alarm_minute, alarm_enabled);

    if (alarm_ringing)
    {
        bool tilt = ((second % 2) == 0);
        draw_bell(tilt);
    } else 
    {
        clear_bell();
    }

    draw_status(gps_found, alarm_enabled, alarm_ringing);
}