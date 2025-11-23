#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/time.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"

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
const        char * default_audio_path = DEFAULT_AUDIO_PATH;
const        char * audio_path;
             bool   audio_file_open = false;    // Is a file open?
              FIL   audio_file;                 // Pointer to file on drive. NOT the read/write pointer.
    file_header_t   file_header;                // Struct for file headers.      
     wav_format_t   wav_format;                 // Struct for wave format headers.

// Circular buffer to hold audio samples read from SD card
#ifndef DOUBLE_BUFFER
          int16_t   audio_buffer   [AUDIO_BUFFER_LEN] = {};
#define             audio_buffer_rp    audio_buffer
#define             audio_buffer_wp    audio_buffer
#endif
#ifdef DOUBLE_BUFFER
          int16_t   audio_buffer_w [AUDIO_BUFFER_LEN] = {};
          int16_t   audio_buffer_r [AUDIO_BUFFER_LEN] = {};
          int16_t * audio_buffer_wp = audio_buffer_w;
          int16_t * audio_buffer_rp = audio_buffer_r;
#endif
volatile uint32_t   i_audio_buf_r   = 0;
volatile uint32_t   i_audio_buf_w   = 0;

// Threshold to trigger start loading and stop loading more audio data
// const    uint16_t   LOAD_THRES      = AUDIO_BUFFER_LEN * LOAD_THRES_RATIO;

// PWM Settings
            float   audio_pwm_psc   = -1.0;     // Fractional prescaler
         uint16_t   audio_pwm_top   = -1;       // Actual top value
            float   audio_pwm_frq   = -1.0;     // Actual frequency
            float   audio_pwm_scale = -1.0;     // Actual top / bit depth top
            float   audio_base_vol  =  1.0;     // Actual top / bit depth top
volatile uint32_t * cc_reg          = &(pwm_hw -> slice[AUDIO_PWM_SLICE].cc);

// So that core1 routine knows if they should update buffer.
            bool    audio_playing   = false;
            bool    audio_load_flag = true;

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

void              close_sd_audio_file() {
    // Close the currently open audio file, if any
    if (!audio_file_open) { return; }
    f_close(&audio_file);
    audio_file_open = false;
}

audio_file_result reopen_sd_audio_file() {
    uint32_t data_offset = f_tell(&audio_file);
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

    printf("\n  (SUCCESS) open_sd_audio_file: Opened File `%s`\n", filename);

    // Parse Headers
    audio_file_result wfr = wav_parse_headers();

    if (wfr == SUCCESS) {
        audio_file_open = true;
        printf("\n  ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓");
        printf("\n  ┃ fmt type          ┃ %8s     ┃",   wav_format.fmt_type == 1 ? "     PCM" : "        ");
        printf("\n  ┃ num channels      ┃ %8s     ┃",   wav_format.num_channels == 2 ? "  STEREO" : "    MONO");
        printf("\n  ┃ sample rate       ┃ %8.3f kHz ┃", wav_format.sample_rate * 1e-3f);
        printf("\n  ┃ bytes/sec         ┃ %8ld     ┃",  wav_format.byte_rate);
        printf("\n  ┃ bytes/sample      ┃ %8d     ┃",   wav_format.bytes_per_ts);
        printf("\n  ┃ bits_per_samp     ┃ %8d     ┃",   wav_format.bits_per_samp);
        printf("\n  ┃ data_offset_base  ┃ %8ld     ┃",  wav_format.data_offset_base);
        printf("\n  ┃ file_size         ┃ %8.3f MiB ┃", file_header.file_size * 1e-6f);
        printf("\n  ┃ data_size         ┃ %8ld B   ┃",  wav_format.data_size);
        printf("\n  ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛");
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
    
    // Enforce mono audio
    if (wav_format.num_channels != 1) {
        printf("(ERROR) open_sd_audio_file: We only support MONO audio files right now!\n");
        return ERR_UNSUPPORTED_TYPE;
    }

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

    return SUCCESS;
}

void              audio_file_lseek(UINT b) {
    f_lseek(&audio_file, b);
}

//   SD -> BUFFER   //

#ifndef DOUBLE_BUFFER
uint16_t          get_buff_readable() {
    // Get readable space in audio buffer
    // If write ptr = read ptr, then assume full
    if (i_audio_buf_w <= i_audio_buf_r) {
        return AUDIO_BUFFER_LEN - (i_audio_buf_r - i_audio_buf_w);
    } else {
        return i_audio_buf_w - i_audio_buf_r;
    }
}
uint16_t          get_buff_writeable() {
    // Get writeable space in audio buffer
    // If write ptr = read ptr, then assume full
    if (i_audio_buf_w > i_audio_buf_r) {
        return AUDIO_BUFFER_LEN - (i_audio_buf_w - i_audio_buf_r);
    } else {
        return i_audio_buf_r - i_audio_buf_w;
    }
}

audio_file_result fill_audio_buffer() {
#ifdef FILL_BUFFER_ERRCHECK
    if (!audio_file_open)  { return ERR_NO_FILE_OPEN; }
#endif

    // Async func: store things that might change due to core 0 execution.
    uint16_t i0_audio_buf_r = i_audio_buf_r;

    // Compute number of writeable positions in circular buffer
    uint32_t buff_avail = (i_audio_buf_w > i0_audio_buf_r)
        ? AUDIO_BUFFER_LEN - (i_audio_buf_w - i0_audio_buf_r)
        : i0_audio_buf_r - i_audio_buf_w;

    // Don't copy anything if buffer is full
    if (!buff_avail) { return ERR_BUFF_FULL; }

    FRESULT fr;
    UINT    br, br2;
    
    // If read ptr is ahead of write ptr, copy from write ptr to read ptr.
    if (i_audio_buf_w <= i0_audio_buf_r) {
        uint32_t copy_size = buff_avail << 1;
        fr = f_read(&audio_file, (char *) (audio_buffer + i_audio_buf_w), copy_size, &br);
#ifdef FILL_BUFFER_ERRCHECK
        if (fr) { printf("(ERROR) fill_audio_buffer\n    "); print_error(fr, "FatFS says"); }
        if (br < copy_size) { goto eof_reached; }
#endif
    }

    // if read ptr is behind write ptr,
    // copy from write ptr to end, then loop around and copy to read ptr.
    else {
        uint32_t copy_size_hi = (AUDIO_BUFFER_LEN - i_audio_buf_w) << 1;
        uint32_t copy_size_lo = (i0_audio_buf_r) << 1;
        
        fr = f_read(&audio_file, (char *) (audio_buffer + i_audio_buf_w), copy_size_hi, &br);
#ifdef FILL_BUFFER_ERRCHECK
        if (fr) { printf("(ERROR) fill_audio_buffer\n    "); print_error(fr, "FatFS says"); }
        if (br < copy_size_hi) { goto eof_reached; }
#endif

        fr = f_read(&audio_file, (char *) audio_buffer, copy_size_lo, &br2);
#ifdef FILL_BUFFER_ERRCHECK
        if (fr) { printf("(ERROR) fill_audio_buffer\n    "); print_error(fr, "FatFS says"); }
        if ((br += br2) < copy_size) { goto eof_reached; }
#endif
    }

    // Update write ptr position!
    i_audio_buf_w = i0_audio_buf_r;  // (i_audio_buf_w + buff_avail) & (AUDIO_BUFFER_LEN - 1); // buff_avail mod 4096

    return SUCCESS;

    // if no shi left to copy
#ifdef FILL_BUFFER_ERRCHECK
    eof_reached:
    i_audio_buf_w = (i_audio_buf_w + (br >> 1)) & (AUDIO_BUFFER_LEN - 1); // add however many was copied
    core1_ready   = true;
    return AUDIO_READ_EOF;
#endif
}

audio_file_result add1_audio_buffer() {
    if (i_audio_buf_r == i_audio_buf_w) { return ERR_BUFF_FULL; }

    UINT br; f_read(&audio_file, (char *) (audio_buffer + i_audio_buf_w), 2, &br); if (br < 2) { goto eof_reached; }

    i_audio_buf_w = (i_audio_buf_w + 1) & (AUDIO_BUFFER_LEN - 1);
    return SUCCESS;

    eof_reached:
    return AUDIO_READ_EOF;
}
#endif

void              core1_maintain_audio_buff_routine() {
    // Call on each loop of core1 main

    // Is audio even playing?
    if ( !audio_playing ) { return; }
    if ( !audio_load_flag ) { return; }

    // Here's your data, bb (˶˘ ³˘)♡
    #if AUDIO_BUFFER_LEN <  512
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x0000), AUDIO_BUFFER_LEN << 1, NULL);
    #endif
    #if AUDIO_BUFFER_LEN >= 512
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x0000), 1024, NULL);
    #endif
    #if AUDIO_BUFFER_LEN >= 1024
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x0200), 1024, NULL);
    #endif
    #if AUDIO_BUFFER_LEN >= 2048
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x0400), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x0600), 1024, NULL);
    #endif
    #if AUDIO_BUFFER_LEN >= 4096
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x0800), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x0A00), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x0C00), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x0E00), 1024, NULL);
    #endif
    #if AUDIO_BUFFER_LEN >= 8192
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x1000), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x1200), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x1400), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x1600), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x1800), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x1A00), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x1C00), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x1E00), 1024, NULL);
    #endif
    #if AUDIO_BUFFER_LEN >= 16384
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2000), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2200), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2400), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2600), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2800), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2A00), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2C00), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2E00), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2000), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2200), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2400), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2600), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2800), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2A00), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2C00), 1024, NULL);
        f_read(&audio_file, (char *) (audio_buffer_wp + 0x2E00), 1024, NULL);
    #endif

    if ( audio_file.fptr >= file_header.file_size ) {
        stop_audio_playback();
        close_sd_audio_file();
    }
    audio_load_flag = false;
}

//  BUFFER -> PWM   //

// /* WITH BUFFER VERSION
void              step_audio_isr() {
    // isr to read next sample from audio_buffer
    pwm_hw -> intr  |= (1 << AUDIO_PWM_SLICE);
    
    // uint16_t buff_readable = get_buff_readable();
    // if (buff_readable <= 1) {
    //     printf("(ERROR) step_audio_isr: Buff Empty.\n");
    //     return;
    // }

    int16_t samp    = (audio_buffer_rp[i_audio_buf_r]);               // Retrieve FIFO
    int16_t scaled  = (int16_t)round(samp * audio_pwm_scale);           // Scale between -top and top

    if (scaled > 0) {
        *cc_reg     = scaled;
    } else {
        *cc_reg     = (-scaled) << 16;
    }

    ++i_audio_buf_r;

    if (i_audio_buf_r >= AUDIO_BUFFER_LEN) {
        i_audio_buf_r   = 0;
        #ifdef DOUBLE_BUFFER
        int16_t * temp  = audio_buffer_wp;
        audio_buffer_wp = audio_buffer_rp;
        audio_buffer_rp = temp;
        audio_load_flag = true;
        #endif
    }

    #ifndef DOUBLE_BUFFER
    if (i_audio_buf_r == LOAD_WHEN) {
        audio_load_flag = true;
    }
    #endif
}
// */

/* NO BUFFER VERSION
uint8_t downsamp_counter = 0;
void              step_audio_isr() {
    // isr to read next sample from audio_buffer
    pwm_hw -> intr  |= (1 << AUDIO_PWM_SLICE);                          // ch 10
    if ( !audio_playing ) { return; }

    // update every other PWM cycle for example.
#ifdef DOWNSAMPLE
    // if (downsamp_counter++ >= DOWNSAMPLE) { downsamp_counter = 0; } else { return; }
    // audio_file.fptr += (DOWNSAMPLE - 1) << 1;
    if (downsamp_counter ^= 1) return;
    audio_file.fptr += 2;
    // if ((downsamp_counter = (downsamp_counter + 1) & 0b11) != 0) { return; }
    // audio_file.fptr += 6;
#endif

    if ( audio_file.fptr >= file_header.file_size ) {
        stop_audio_playback();
    }
    
    int16_t samp    = 0; f_read(&audio_file, (char *) &samp, 2, NULL);  // Retrieve next sample
    int16_t scaled  = (int16_t)round(samp * audio_pwm_scale);           // Scale between -top and top

    if (scaled > 0) {
        *cc_reg     = scaled;
    } else {
        *cc_reg     = (-scaled) << 16;
    }
}
// */

void              configure_audio_play() {
    // Initialize DMA to copy from spi to pwm
    gpio_set_function(AUDIO_PWM_PIN_L, GPIO_FUNC_PWM);
    gpio_set_function(AUDIO_PWM_PIN_H, GPIO_FUNC_PWM);

    // Set wrap interrupt
    pwm_set_irq_enabled(AUDIO_PWM_SLICE, true);
    irq_set_exclusive_handler(AUDIO_PWM_INT_NUM, &step_audio_isr);
    irq_set_enabled(AUDIO_PWM_INT_NUM, true);
    irq_set_priority(AUDIO_PWM_INT_NUM, 0);

    // Set PWM Freq and Top

    audio_pwm_psc = PWM_CLK_PSC(wav_format.sample_rate, wav_format.bits_per_samp);      // Estimate psc...
    audio_pwm_psc = CLIP(audio_pwm_psc, 1.f, 256.f);
    pwm_set_clkdiv(AUDIO_PWM_SLICE, audio_pwm_psc);
        uint32_t real_div_reg = pwm_hw -> slice[AUDIO_PWM_SLICE].div;
    audio_pwm_psc = FIXED_8p4_TO_FLOAT(real_div_reg);                                   // Then get true value

    float best_top = PWM_EST_TOP(wav_format.sample_rate, audio_pwm_psc);                // Get theoretical top
    audio_pwm_top  = (uint16_t)roundf(best_top);                                        // Used top value
    pwm_set_wrap(AUDIO_PWM_SLICE, audio_pwm_top - 1);                                   // MY TOP var is the 100% duty cc value, the hardware TOP is this minus 1.

    audio_pwm_frq  = PWM_GET_FRQ(audio_pwm_psc, audio_pwm_top);                         // Used freq (will differ slightly from wav_format.bits_per_samp)
    audio_pwm_scale = audio_base_vol *
        (float)(audio_pwm_top << 1) / (1 << wav_format.bits_per_samp);                  // Scale between -audio_pwm_top and +audio_pwm_top
    
    printf("\n\n  configure_audio_play: wants to note Bit Depth and Sampling Rate Changes:\n    File requested top = %d at freq = %.3f kHz,\n    However, best top achieveable = %.3f using psc. of %.2f -> %.2f MHz PWM clk,\n    Using top %d with sample rate = %.3f kHz!\n",
        1 << wav_format.bits_per_samp, wav_format.sample_rate * 1e-3f,
        best_top, audio_pwm_psc, (float)BASE_CLK / audio_pwm_psc * 1e-6f,
        audio_pwm_top, audio_pwm_frq * 1e-3f
    );

    // Init both channels to 0 duty
    pwm_set_both_levels(AUDIO_PWM_SLICE, 0, 0);
    stop_audio_playback();
}

void              start_audio_playback() {
    gpio_set_function(AUDIO_PWM_PIN_L, GPIO_FUNC_PWM);
    gpio_set_function(AUDIO_PWM_PIN_H, GPIO_FUNC_PWM);
    pwm_set_enabled(AUDIO_PWM_SLICE, true);
    audio_playing = true;
}

void              stop_audio_playback() {
    gpio_set_function(AUDIO_PWM_PIN_L, GPIO_FUNC_SIO);
    gpio_set_function(AUDIO_PWM_PIN_H, GPIO_FUNC_SIO);
    gpio_put(AUDIO_PWM_PIN_L, 0);
    gpio_put(AUDIO_PWM_PIN_H, 0);
    pwm_set_enabled(AUDIO_PWM_SLICE, false);
    audio_playing = false;
}