#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"

#include <hardware/spi.h>
#include <hardware/pwm.h>

#include "const.h"
#include "sd.h"
#include "sd_ff.h"
#include "sd_sdcard.h"
#include "sd_diskio.h"
#include "sd_audio.h"

// INITIALIZE GLOBS  //

// Audio File
const char *        default_audio_path = DEFAULT_AUDIO_PATH;
const char *        audio_path;
bool                audio_file_open = false;    // Is a file open?
FIL                 audio_file;                 // Pointer to file on drive. NOT the read/write pointer.
FSIZE_t             data_offset = 0;            // Read/Write pointer to temporarily hold the fatFS file pointer.
file_header_t       file_header;                // Struct for file headers.      
wav_format_t        wav_format;                 // Struct for wave format headers.

// Circular buffer to hold audio samples read from SD card
          int16_t   audio_buffer [AUDIO_BUFFER_LEN] = {};
volatile uint16_t   i_audio_buf_w   = 0;
volatile uint16_t   i_audio_buf_r   = 0;

// Threshold to trigger start loading and stop loading more audio data
const    uint16_t   LOAD_WHEN       = AUDIO_BUFFER_LEN >> 2;

// HELPER FUNCTIONS //

uint32_t to_little_endian(uint32_t x) {
#ifdef READ_BIG_ENDIAN
    x ^= (x << 24);
    x ^= (x >> 24);
    x ^= (x << 24);
    x ^= ((x >> 8) << 24) >> 8;
    x ^= ((x << 8) >> 24) << 8;
    x ^= ((x >> 8) << 24) >> 8;
#endif
    return x;
}

uint16_t to_little_endian16(uint16_t x) {
#ifdef READ_BIG_ENDIAN
    x ^= (x >> 8);
    x ^= (x << 8);
    x ^= (x << 8);
#endif
    return x;
}

//   FILE ACCESS    //

uint32_t get_data_offset() {
    return data_offset;
}

void close_sd_audio_file() {
    // Close the currently open audio file, if any
    if (!audio_file_open) { return; }
    f_close(&audio_file);
    audio_file_open = false;
}

audio_file_result reopen_sd_audio_file() {
    data_offset = f_tell(&audio_file);
    f_close(&audio_file);

    FRESULT fr;
    if ( (fr = f_open(&audio_file, audio_path, FA_READ)) ) {
        print_error(fr, "(ERROR) open_sd_audio_file: ");
        return ERR_ON_FILE_OPEN;
    }

    f_lseek(&audio_file, data_offset);
    return SUCCESS;
}

audio_file_result open_sd_audio_file(const char* filename) {
    // Close any open audio files first
    close_sd_audio_file();

    audio_path = filename;

    // Open the audio file on SD card for reading
    // Store in audio_file global variable
    FRESULT fr;
    if ( (fr = f_open(&audio_file, filename, FA_READ)) ) {
        print_error(fr, "(ERROR) open_sd_audio_file: ");
        return ERR_ON_FILE_OPEN;
    }

#ifdef PRINT_SUCCESS
    printf("open_sd_audio_file: Opened File %s\n", filename);
#endif

    // Parse Headers
    audio_file_result wfr = wav_parse_headers();

    if (wfr == SUCCESS) {
        audio_file_open = true;
    } else {
        f_close(&audio_file);
    }
    return wfr;
}

audio_file_result wav_parse_headers() {

    // Parse file headers
    FRESULT fr;                             // result of each f_read operation
    UINT    br;                             // bytes read per f_read operation
    UINT    header_br = 0;                  // total bytes read
    char    temp_buf[5] = {0,0,0,0,0};      // temporary register for finding delimiters "fmt\0" and "data"
    
    // Read first 12 bytes
    if ( (fr = f_read(&audio_file, &file_header, 12, &br)) ) {
        printf("(ERROR) open_sd_audio_file: Could not read file headers!\n");
        printf("    "); print_error(fr, "fatFS says");
        return ERR_BAD_FILE_HEADER;
    }
    header_br += br;

    // Convert big to little endian if needed
    file_header.file_size = to_little_endian(file_header.file_size);

    // Only do WAVE files
    if (strcmp(file_header.ftype_header, "WAVE")) {
        printf("(ERROR) open_sd_audio_file: Only .wav files supported.\n");
        return ERR_UNSUPPORTED_TYPE;
    }
    // Set file format to wave format struct
    file_header.file_format = &wav_format;

    // Seek to end of fmt\0 block, we don't care about any other metadata.
    do {
        if (header_br > 100) {
            printf("(ERROR) open_sd_audio_file: File header end marker not found!\n");
            return ERR_BAD_FILE_HEADER;
        }
        if ( (fr = f_read(&audio_file, &temp_buf, 4, &br)) ) {
            printf("(ERROR) open_sd_audio_file: Could not read file headers!\n");
            printf("    "); print_error(fr, "fatFS says");
            return ERR_BAD_FILE_HEADER;
        }
        header_br += br;
    } while (!strcmp(temp_buf, "fmt\0"));

    // Read header size
    uint32_t header_size;
    if ( (fr = f_read(&audio_file, (char *)(&header_size), 4, &br)) ) {
        printf("(ERROR) open_sd_audio_file: Could not read file headers!\n");
        printf("    "); print_error(fr, "fatFS says");
        return ERR_BAD_FILE_HEADER;
    }
    // Adjust endianness
    header_size = to_little_endian(header_size);

    if (header_size != header_br){
        printf("(WARN) open_sd_audio_file: Specified file header size != Read file header size\n");
    }
    header_br += br;

    // Parse wav headers - next 20 bytes correspond to struct wav_fmt_t 
    uint32_t wav_header_offset = header_br;

    if ( (fr = f_read(&audio_file, &wav_format, 16, &br)) ) {
        printf("(ERROR) open_sd_audio_file: Could not read WAV format header!\n");
        printf("    "); print_error(fr, "fatFS says");
        return ERR_BAD_WAV_HEADER;
    }
    header_br += br;

    // Adjust endianness
    wav_format.fmt_type      = to_little_endian16(wav_format.fmt_type     );
    wav_format.num_channels  = to_little_endian16(wav_format.num_channels );
    wav_format.sample_rate   = to_little_endian  (wav_format.sample_rate  );
    wav_format.byte_rate     = to_little_endian16(wav_format.byte_rate    );
    wav_format.bytes_per_ts  = to_little_endian16(wav_format.bytes_per_ts );
    wav_format.bits_per_samp = to_little_endian  (wav_format.bits_per_samp);      

    // Look for "data" marker
    do {
        if (header_br - wav_header_offset > 100) {
            printf("(ERROR) open_sd_audio_file: Data Start Marker not Found!\n");
            return ERR_BAD_WAV_HEADER;
        }
        if ( (fr = f_read(&audio_file, &temp_buf, 4, &br)) ) {
            printf("(ERROR) open_sd_audio_file: Could not read WAV format header!\n");
            printf("    "); print_error(fr, "fatFS says");
            return ERR_BAD_WAV_HEADER;
        }
        header_br += br;
        if (!strcmp(temp_buf, "data\0")){
            break;
        }
        // Go in half-words.
        header_br -= 2;
        f_lseek(&audio_file, header_br);    // Backtrack by 2 and copy again!
    } while (1);
    
    // Store file data size
    if ( (fr = f_read(&audio_file, (char *)&(wav_format.data_size), 4, &br)) ) {
        printf("(ERROR) open_sd_audio_file: Could not read WAV format header!\n");
        printf("    "); print_error(fr, "fatFS says");
        return ERR_BAD_WAV_HEADER;
    }
    header_br += br;

    // Adjust endianness
    wav_format.data_size = to_little_endian(wav_format.data_size);

    // Record where data starts
    wav_format.data_offset_base = header_br;
    data_offset = f_tell(&audio_file);

    return SUCCESS;
}

//   SD -> BUFFER   //

uint16_t get_buff_avail() {
    // Get available space in audio buffer
    if (i_audio_buf_w >= i_audio_buf_r) {
        return AUDIO_BUFFER_LEN - (i_audio_buf_w - i_audio_buf_r);
    } else {
        return i_audio_buf_r - i_audio_buf_w;
    }
}

audio_file_result fill_audio_buffer() {
    if (!audio_file_open)  { return ERR_NO_FILE_OPEN; }

    // reopen_sd_audio_file();
    // f_lseek(&audio_file, 0);

    uint32_t buff_avail = get_buff_avail();
    if (!buff_avail) { return ERR_BUFF_FULL; }
    
    FRESULT fr;
    UINT    br;
    UINT    br2;

    uint32_t i0_audio_buf_w = i_audio_buf_w;
    i_audio_buf_w = (i_audio_buf_w + buff_avail) & (AUDIO_BUFFER_LEN - 1); // buff_avail mod 4096

    uint32_t copy_size = buff_avail << 1;
    if (i_audio_buf_w >= i_audio_buf_r) {
        if ((fr = f_read(&audio_file, (char *) &audio_buffer, copy_size, &br)))
            { printf("(ERROR) fill_audio_buffer\n    "); print_error(fr, "FatFS says"); }
        if (br < copy_size) { goto eof_reached; }
    } else {
        uint32_t copy_size_hi = (AUDIO_BUFFER_LEN - i0_audio_buf_w) << 1;
        uint32_t copy_size_lo = (i_audio_buf_w) << 1;

        if ((fr = f_read(&audio_file, (char *) &audio_buffer, copy_size_hi, &br)))
            { printf("(ERROR) fill_audio_buffer\n    "); print_error(fr, "FatFS says"); }
        if (br < copy_size_hi) { goto eof_reached; }

        if ((fr = f_read(&audio_file, (char *) &audio_buffer, copy_size_lo, &br2)))
            { printf("(ERROR) fill_audio_buffer\n    "); print_error(fr, "FatFS says"); }
        if ((br += br2) < copy_size) { goto eof_reached; }
    }

    data_offset   = f_tell(&audio_file);
    return SUCCESS;

    // if no shi left to copy
    eof_reached:
    i_audio_buf_w = (i0_audio_buf_w + (br >> 1)) & (AUDIO_BUFFER_LEN - 1); // rollback change
    data_offset   = f_tell(&audio_file);
    return AUDIO_READ_EOF;
}

void stop_sd_audio_read() {
    // Stop the DMA
    
    // Stop reading audio data from SD card
    // Triggered when buffer is full or EOF reached.

}

//  BUFFER -> PWM   //

void step_audio() {
    // isr to read next sample from audio_buffer
    
    i_audio_buf_r = (i_audio_buf_r + 1) & (AUDIO_BUFFER_LEN - 1);
}

// Initialize this on core 1
uint8_t configure_audio_play () {
    // Initialize DMA to copy from spi to pwm
    gpio_set_function(AUDIO_PWM_PIN_L, GPIO_FUNC_PWM);
    gpio_set_function(AUDIO_PWM_PIN_H, GPIO_FUNC_PWM);

    // Set wrap interrupt
    pwm_set_irq_enabled(AUDIO_PWM_SLICE, true);
    irq_set_exclusive_handler(AUDIO_PWM_INT_NUM, &step_audio);
    irq_set_enabled(AUDIO_PWM_INT_NUM, true);
    irq_set_priority(AUDIO_PWM_INT_NUM, 0);

    pwm_set_wrap(AUDIO_PWM_SLICE, PWM_CLK_PSC(wav_format.sample_rate, wav_format.bits_per_samp));
}

void start_audio_playback() {
    // Start audio playback using the audio buffer
    // Let freerun, will play audio as long as buffer_avail > 0
}

void stop_audio_playback() {

}