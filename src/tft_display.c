#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "const.h"
#include "tft_display.h"

// Font for alarm time and bottom text
// Space
static const uint8_t CHAR_SPACE[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

// Numbers
static const uint8_t CHAR_DIGITS[10][5] = {
    // '0'
    {0x3E, 0x41, 0x49, 0x41, 0x3E},
    // '1'
    {0x00, 0x42, 0x7F, 0x40, 0x00},
    // '2'
    {0x42, 0x61, 0x51, 0x49, 0x46},
    // '3'
    {0x22, 0x41, 0x49, 0x49, 0x36},
    // '4'
    {0x18, 0x14, 0x12, 0x7F, 0x10},
    // '5'
    {0x27, 0x45, 0x45, 0x45, 0x39},
    // '6'
    {0x3C, 0x4A, 0x49, 0x49, 0x30},
    // '7'
    {0x01, 0x71, 0x09, 0x05, 0x03},
    // '8'
    {0x36, 0x49, 0x49, 0x49, 0x36},
    // '9'
    {0x06, 0x49, 0x49, 0x29, 0x1E},
};

// Colon
static const uint8_t CHAR_COLON[5] = {0x00, 0x00, 0x36, 0x00, 0x00};

// Letters
static const uint8_t CHAR_A[5] = {0x7E, 0x09, 0x09, 0x09, 0x7E};
static const uint8_t CHAR_B[5] = {0x7F, 0x49, 0x49, 0x49, 0x36};
static const uint8_t CHAR_C[5] = {0x3E, 0x41, 0x41, 0x41, 0x22};
static const uint8_t CHAR_D[5] = {0x7F, 0x41, 0x41, 0x41, 0x3E};
static const uint8_t CHAR_E[5] = {0x7F, 0x49, 0x49, 0x49, 0x41};
static const uint8_t CHAR_F[5] = {0x7F, 0x09, 0x09, 0x09, 0x01};
static const uint8_t CHAR_G[5] = {0x3E, 0x41, 0x49, 0x49, 0x3A};
static const uint8_t CHAR_H[5] = {0x7F, 0x08, 0x08, 0x08, 0x7F};
static const uint8_t CHAR_I[5] = {0x41, 0x41, 0x7F, 0x41, 0x41};
static const uint8_t CHAR_J[5] = {0x02, 0x41, 0x41, 0x3F, 0x01};
static const uint8_t CHAR_K[5] = {0x7F, 0x08, 0x14, 0x22, 0x41};
static const uint8_t CHAR_L[5] = {0x7F, 0x40, 0x40, 0x40, 0x40};
static const uint8_t CHAR_M[5] = {0x7F, 0x02, 0x04, 0x02, 0x7F};
static const uint8_t CHAR_N[5] = {0x7F, 0x02, 0x04, 0x08, 0x7F};
static const uint8_t CHAR_O[5] = {0x3E, 0x41, 0x41, 0x41, 0x3E};
static const uint8_t CHAR_P[5] = {0x7F, 0x09, 0x09, 0x09, 0x06};
static const uint8_t CHAR_Q[5] = {0x3E, 0x41, 0x51, 0x21, 0x5E};
static const uint8_t CHAR_R[5] = {0x7F, 0x09, 0x19, 0x29, 0x46};
static const uint8_t CHAR_S[5] = {0x46, 0x49, 0x49, 0x49, 0x31};
static const uint8_t CHAR_T[5] = {0x01, 0x01, 0x7F, 0x01, 0x01};
static const uint8_t CHAR_U[5] = {0x7F, 0x40, 0x40, 0x40, 0x3F};
static const uint8_t CHAR_V[5] = {0x0F, 0x30, 0x40, 0x30, 0x0F};
static const uint8_t CHAR_W[5] = {0x7F, 0x20, 0x18, 0x20, 0x7F};
static const uint8_t CHAR_X[5] = {0x63, 0x14, 0x08, 0x14, 0x63};
static const uint8_t CHAR_Y[5] = {0x07, 0x08, 0x70, 0x08, 0x07};
static const uint8_t CHAR_Z[5] = {0x61, 0x51, 0x49, 0x45, 0x43};

static const uint8_t *get_char(char c)
{
    // make uppercase
    if (c >= 'a' && c <= 'z') {
        c = (char)(c - 'a' + 'A');
    }

    if (c == ' ')
        return CHAR_SPACE;

    if (c >= '0' && c <= '9')
        return CHAR_DIGITS[c - '0'];

    switch (c) {
    case ':': return CHAR_COLON;
    case 'A': return CHAR_A;
    case 'B': return CHAR_B;
    case 'C': return CHAR_C;
    case 'D': return CHAR_D;
    case 'E': return CHAR_E;
    case 'F': return CHAR_F;
    case 'G': return CHAR_G;
    case 'H': return CHAR_H;
    case 'I': return CHAR_I;
    case 'J': return CHAR_J;
    case 'K': return CHAR_K;
    case 'L': return CHAR_L;
    case 'M': return CHAR_M;
    case 'N': return CHAR_N;
    case 'O': return CHAR_O;
    case 'P': return CHAR_P;
    case 'Q': return CHAR_Q;
    case 'R': return CHAR_R;
    case 'S': return CHAR_S;
    case 'T': return CHAR_T;
    case 'U': return CHAR_U;
    case 'V': return CHAR_V;
    case 'W': return CHAR_W;
    case 'X': return CHAR_X;
    case 'Y': return CHAR_Y;
    case 'Z': return CHAR_Z;
    default: // Anything else becomes space
        return CHAR_SPACE;
    }
}

static inline void tft_select()   
{ 
    gpio_put(TFT_PIN_CS, 0); 
}
static inline void tft_deselect() 
{ 
    gpio_put(TFT_PIN_CS, 1); 
}
static inline void tft_dc_command() 
{ 
    gpio_put(TFT_PIN_DC, 0); 
}
static inline void tft_dc_data()    
{ 
    gpio_put(TFT_PIN_DC, 1); 
}

static void tft_write_cmd(uint8_t cmd)
{
    tft_select();
    tft_dc_command();
    spi_write_blocking(TFT_SPI, &cmd, 1);
    tft_deselect();
}

static void tft_write_data(const uint8_t *data, size_t len)
{
    tft_select();
    tft_dc_data();
    spi_write_blocking(TFT_SPI, data, len);
    tft_deselect();
}

static void tft_write_data8(uint8_t data)
{
    tft_write_data(&data, 1);
}

// Make drawing window x0,y0 (inclusive) to x1,y1 (inclusive)
static void tft_set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t buf[4];

    // Column address
    tft_write_cmd(0x2A);
    buf[0] = (x0 >> 8); buf[1] = x0 & 0xFF;
    buf[2] = (x1 >> 8); buf[3] = x1 & 0xFF;
    tft_write_data(buf, 4);

    // Row address
    tft_write_cmd(0x2B);
    buf[0] = (y0 >> 8); buf[1] = y0 & 0xFF;
    buf[2] = (y1 >> 8); buf[3] = y1 & 0xFF;
    tft_write_data(buf, 4);

    // Memory write
    tft_write_cmd(0x2C);
}

void tft_init_pins()
{
    // SPI pins
    gpio_set_function(TFT_PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(TFT_PIN_MOSI, GPIO_FUNC_SPI);

    // Control pins
    gpio_init(TFT_PIN_CS);
    gpio_set_dir(TFT_PIN_CS, GPIO_OUT);
    tft_deselect();

    gpio_init(TFT_PIN_DC);
    gpio_set_dir(TFT_PIN_DC, GPIO_OUT);
    gpio_put(TFT_PIN_DC, 0);

    gpio_init(TFT_PIN_RST);
    gpio_set_dir(TFT_PIN_RST, GPIO_OUT);
    gpio_put(TFT_PIN_RST, 1);

    spi_init(TFT_SPI, 10 * 1000 * 1000);
    spi_set_format(TFT_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

void tft_init()
{
    gpio_put(TFT_PIN_RST, 0);
    sleep_ms(20);
    gpio_put(TFT_PIN_RST, 1);
    sleep_ms(120);

    tft_write_cmd(0x01);
    sleep_ms(5);

    // Display off
    tft_write_cmd(0x28);

    // Power control A
    uint8_t data1[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
    tft_write_cmd(0xCB);
    tft_write_data(data1, sizeof(data1));

    // Power control B
    uint8_t data2[] = {0x00, 0xC1, 0x30};
    tft_write_cmd(0xCF);
    tft_write_data(data2, sizeof(data2));

    // Driver timing control A
    uint8_t data3[] = {0x85, 0x00, 0x78};
    tft_write_cmd(0xE8);
    tft_write_data(data3, sizeof(data3));

    // Driver timing control B
    uint8_t data4[] = {0x00, 0x00};
    tft_write_cmd(0xEA);
    tft_write_data(data4, sizeof(data4));

    // Power on sequence control
    uint8_t data5[] = {0x64, 0x03, 0x12, 0x81};
    tft_write_cmd(0xED);
    tft_write_data(data5, sizeof(data5));

    // Pump ratio control
    tft_write_cmd(0xF7);
    tft_write_data8(0x20);

    // Power control, VRH[5:0]
    tft_write_cmd(0xC0);
    tft_write_data8(0x23);

    // Power control, SAP[2:0]; BT[3:0]
    tft_write_cmd(0xC1);
    tft_write_data8(0x10);

    // VCM control
    uint8_t data6[] = {0x3E, 0x28};
    tft_write_cmd(0xC5);
    tft_write_data(data6, sizeof(data6));

    // VCM control 2
    tft_write_cmd(0xC7);
    tft_write_data8(0x86);

    // Memory Access Control – portrait orientation
    tft_write_cmd(0x36);
    tft_write_data8(0x48); // MX=0, MY=1, BGR=1 (change if rotated)

    // Pixel format is 16 bits
    tft_write_cmd(0x3A);
    tft_write_data8(0x55); // 16-bit per pixel

    // Frame rate control
    tft_write_cmd(0xB1);
    uint8_t data7[] = {0x00, 0x18};
    tft_write_data(data7, sizeof(data7));

    // Display function control
    uint8_t data8[] = {0x08, 0x82, 0x27};
    tft_write_cmd(0xB6);
    tft_write_data(data8, sizeof(data8));

    // 3G gamma control disable
    tft_write_cmd(0xF2);
    tft_write_data8(0x00);

    // Gamma curve select
    tft_write_cmd(0x26);
    tft_write_data8(0x01);

    // Positive gamma correction
    uint8_t gm1[] = {0x0F,0x31,0x2B,0x0C,0x0E,0x08,0x4E,0xF1,0x37,0x07,0x10,0x03,0x0E,0x09,0x00};
    tft_write_cmd(0xE0);
    tft_write_data(gm1, sizeof(gm1));

    // Negative gamma correction
    uint8_t gm2[] = {0x00,0x0E,0x14,0x03,0x11,0x07,0x31,0xC1,0x48,0x08,0x0F,0x0C,0x31,0x36,0x0F};
    tft_write_cmd(0xE1);
    tft_write_data(gm2, sizeof(gm2));

    // Exit sleep
    tft_write_cmd(0x11);
    sleep_ms(120);

    // Display On
    tft_write_cmd(0x29);
    sleep_ms(20);

    // Clear display
    fill_screen(TFT_BLACK);
}

void fill_screen(uint16_t color)
{
    fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

void fill_rect(int x, int y, int w, int h, uint16_t color)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > TFT_WIDTH)  w = TFT_WIDTH  - x;
    if (y + h > TFT_HEIGHT) h = TFT_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    tft_set_addr_window(x, y, x + w - 1, y + h - 1);

    // Prepare color bytes
    uint8_t high = color >> 8;
    uint8_t low = color & 0xFF;

    tft_select();
    tft_dc_data();

    for (int i = 0; i < w * h; i++) {
        uint8_t pixel[2] = {high, low};
        spi_write_blocking(TFT_SPI, pixel, 2);
    }

    tft_deselect();
}

void draw_char(int x, int y, char c, uint16_t fg, uint16_t bg)
{
    // Char size
    const int char_w = 6;
    const int char_h = 8;

    // If char is outside display, don't draw it
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT || x + char_w <= 0 || y + char_h <= 0) {
        return;
    }

    const uint8_t *letter = get_char(c);

    // Make drawing window for char
    tft_set_addr_window(x, y, x + char_w - 1, y + char_h - 1);

    uint8_t fg_high = fg >> 8;
    uint8_t fg_low = fg & 0xFF;
    uint8_t bg_high = bg >> 8;
    uint8_t bg_low = bg & 0xFF;

    tft_select();
    tft_dc_data();

    // For each row (0-7) and column (0-5)
    for (int row = 0; row < char_h; row++) {
        for (int col = 0; col < char_w; col++) {
            uint8_t pixel_high, pixel_low;

            if (row < 7 && col < 5) {
                uint8_t col_bits = letter[col];
                bool on = (col_bits >> row) & 0x01;
                if (on) {
                    pixel_high = fg_high;
                    pixel_low = fg_low;
                } else {
                    pixel_high = bg_high;
                    pixel_low = bg_low;
                }
            } else {
                // spacing row/col in the background
                pixel_high = bg_high;
                pixel_low = bg_low;
            }

            uint8_t buf[2] = { pixel_high, pixel_low };
            spi_write_blocking(TFT_SPI, buf, 2);
        }
    }

    tft_deselect();
}

void draw_string(int x, int y, const char *string, uint16_t fg, uint16_t bg)
{
    const int char_w = 6;
    while (*string) {
        draw_char(x, y, *string, fg, bg);
        x += char_w;
        string++;
    }
}

void draw_char_scaled(int x, int y, char c, int scale, uint16_t fg, uint16_t bg)
{
    if (scale < 1) scale = 1;

    const uint8_t *letter = get_char(c);

    // For each base pixel
    for (int row = 0; row < TEXT_H; row++) {
        for (int col = 0; col < TEXT_W; col++) {
            bool on = false;

            if (row < 7 && col < 5) {
                uint8_t col_bits = letter[col];
                on = ((col_bits >> row) & 0x01) != 0;
            }

            uint16_t color = on ? fg : bg;

            int px = x + col * scale;
            int py = y + row * scale;
            int pw = scale;
            int ph = scale;

            fill_rect(px, py, pw, ph, color);
        }
    }
}

void draw_string_scaled(int x, int y, const char *string, int scale, uint16_t fg, uint16_t bg)
{
    if (scale < 1) scale = 1;
    int cw = TEXT_W * scale;

    while (*string) {
        draw_char_scaled(x, y, *string, scale, fg, bg);
        x += cw;
        string++;
    }
}