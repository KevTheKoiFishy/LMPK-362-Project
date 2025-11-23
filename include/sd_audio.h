#ifndef  SD_AUDIO_HEADER
#define  SD_AUDIO_HEADER

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"

#include <hardware/spi.h>

#include "const.h"
#include "sd.h"
#include "sd_ff.h"
#include "sd_diskio.h"

// Audio File Helpers
typedef enum uint8_t {
    SUCCESS              = 00,
    ERR_ON_FILE_OPEN     = 01,
    ERR_BAD_FILE_HEADER  = 02,
    ERR_UNSUPPORTED_TYPE = 03,
    ERR_BAD_WAV_HEADER   = 04,
    ERR_NO_FILE_OPEN     = 011,
    ERR_BUFF_FULL        = 012,
    AUDIO_READ_EOF       = 020,
} audio_file_result;

typedef struct {
    uint16_t        fmt_type;
    uint16_t        num_channels;
    uint32_t        sample_rate;
    uint32_t        byte_rate;
    uint16_t        bytes_per_ts;
    uint16_t        bits_per_samp;
    uint32_t        data_offset_base;
    uint32_t        data_size;
} wav_format_t;

typedef wav_format_t file_fmt_t;

typedef struct {
    char            file_type[4];
    FSIZE_t         file_size;
    char            ftype_header[4];
    file_fmt_t *    file_format;
} file_header_t;

// Audio File
// #define             READ_BIG_ENDIAN
#define             DEFAULT_AUDIO_PATH  "AUDIO/SHAUN~17.WAV"

// PWM Playback
#define             CLIP(x, min, max)                   ( x < min ? min : x > max ? max : x )
#define             ReLU(x)                             ( x < 0 ? x : 0 )
#define             PWM_CLK_PSC(sample_rate, bit_depth) ((float)BASE_CLK / (sample_rate << bit_depth))
#define             PWM_EST_TOP(sample_rate, psc)       ((float)BASE_CLK / (sample_rate * psc) )
#define             PWM_GET_FRQ(psc, top)               ((float)BASE_CLK / (psc * top) )
#define             FIXED_8p4_TO_FLOAT(x)               ((float)( ((uint16_t)x >> 4) & 0xFF ) + (float)((uint16_t)x & 0xF) * 0.0625f)
#define             FIXED_16p8_TO_FLOAT(x)              ((float)( ((uint32_t)x >> 8) & 0xFFFF ) + (float)((uint32_t)x & 0xFF) * 0.00390625)

// Buffer
#define             AUDIO_BUFFER_LEN    4096 // Make this a power of 2
// #define             DOWNSAMPLE          2
// #define             LOAD_THRES_RATIO    1 / 2
// #define             LOAD_THRES          420
#define             LOAD_WHEN           0xC00

// Function Declarations
uint32_t            to_little_endian(uint32_t x);
uint16_t            to_little_endian16(uint16_t x);

void                close_sd_audio_file();
audio_file_result   reopen_sd_audio_file();
audio_file_result   reopen_sd_audio_file();
audio_file_result   open_sd_audio_file(const char* filename);
audio_file_result   wav_parse_headers();
void                audio_file_lseek(UINT b);

uint16_t            get_buff_readable();
uint16_t            get_buff_writeable();
audio_file_result   fill_audio_buffer();
audio_file_result   add1_audio_buffer();
void                core1_maintain_audio_buff_routine();

void                step_audio_isr();
void                configure_audio_play();
void                start_audio_playback();
void                stop_audio_playback();

#endif