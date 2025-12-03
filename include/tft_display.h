#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TEXT_W 6
#define TEXT_H 8

// Colors
#define TFT_BLACK   0x0000
#define TFT_WHITE   0xFFFF
#define TFT_BLUE    0x001F
#define TFT_YELLOW  0xFFE0

void tft_init_pins();
void tft_init();
void fill_screen(uint16_t color);
void fill_rect(int x, int y, int w, int h, uint16_t color);
void draw_char(int x, int y, char c, uint16_t fg, uint16_t bg);
void draw_string(int x, int y, const char *string, uint16_t fg, uint16_t bg);
void draw_char_scaled(int x, int y, char c, int scale, uint16_t fg, uint16_t bg);
void draw_string_scaled(int x, int y, const char *string, int scale, uint16_t fg, uint16_t bg);

#endif