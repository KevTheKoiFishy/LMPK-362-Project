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
    SUCCESS              = 0,
    ERR_ON_FILE_OPEN     = 1,
    ERR_BAD_FILE_HEADER  = 2,
    ERR_UNSUPPORTED_TYPE = 3,
    ERR_BAD_WAV_HEADER   = 4,
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
#define             PWM_CLK_PSC         150u
#define             AUDIO_SAMPLE_PERIOD (sample_rate) BASE_CLK / PWM_CLK_PSC / sample_rate

// Buffer
#define             AUDIO_BUFFER_LEN 4096

// Function Declarations
void                close_sd_audio_file();
audio_file_result   open_sd_audio_file(const char* filename);
audio_file_result   wav_parse_headers();

uint16_t            get_buff_avail();
audio_file_result   start_sd_audio_read();
void                stop_sd_audio_read();

uint8_t             configure_audio_dma();
void                start_audio_playback();
void                stop_audio_playback();

#endif