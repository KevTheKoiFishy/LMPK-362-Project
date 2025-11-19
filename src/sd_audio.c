#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"

#include <hardware/spi.h>

#include "const.h"
#include "sd.h"
#include "sd_ff.h"
#include "sd_sdcard.h"
#include "sd_diskio.h"
#include "sd_audio.h"

//////////////////////
// INITIALIZE GLOBS //
//////////////////////

// Audio File
const char *        default_audio_path = DEFAULT_AUDIO_PATH;
bool                audio_file_open = false;    // Is a file open?
FIL                 audio_file;                 // Pointer to file on drive. NOT the read/write pointer.
FSIZE_t             data_offset = 0;            // Read/Write pointer to temporarily hold the fatFS file pointer.
file_header_t       file_header;                // Struct for file headers.      
wav_format_t        wav_format;                 // Struct for wave format headers.

// Circular buffer to hold audio samples read from SD card
const     int16_t   AUDIO_BUFFER [AUDIO_BUFFER_LEN] = {};
volatile uint16_t   I_buffer_write  = 0;
volatile uint16_t   I_buffer_read   = 0;

// Threshold to trigger start loading and stop loading more audio data
const    uint16_t   LOAD_WHEN       = AUDIO_BUFFER_LEN >> 2;

//////////////////////
// HELPER FUNCTIONS //
//////////////////////

uint32_t reverse_endian(uint32_t x) {
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

uint16_t reverse_endian16(uint16_t x) {
#ifdef READ_BIG_ENDIAN
    x ^= (x >> 8);
    x ^= (x << 8);
    x ^= (x << 8);
#endif
    return x;
}

//////////////////////
//   FILE ACCESS    //
//////////////////////

void close_sd_audio_file() {
    // Close the currently open audio file, if any
    if (!audio_file_open) { return; }
    f_close(&audio_file);
    audio_file_open = false;
}

audio_file_result open_sd_audio_file(const char* filename) {
    // Close any open audio files first
    close_sd_audio_file(filename);

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
    file_header.file_size = reverse_endian(file_header.file_size);

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
    header_size = reverse_endian(header_size);
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

    // Convert big to little endian
    wav_format.fmt_type      = reverse_endian16(wav_format.fmt_type     );
    wav_format.num_channels  = reverse_endian16(wav_format.num_channels );
    wav_format.sample_rate   = reverse_endian  (wav_format.sample_rate  );
    wav_format.byte_rate     = reverse_endian16(wav_format.byte_rate    );
    wav_format.bytes_per_ts  = reverse_endian16(wav_format.bytes_per_ts );
    wav_format.bits_per_samp = reverse_endian  (wav_format.bits_per_samp);      

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

    // Big to Little Endian
    wav_format.data_size = reverse_endian(wav_format.data_size);

    // Record where data starts
    wav_format.data_offset_base = header_br;

    return SUCCESS;
}

//////////////////////
//   SD -> BUFFER   //
//////////////////////

uint16_t get_buff_avail() {
    // Get available space in audio buffer
    if (I_buffer_write >= I_buffer_read) {
        return AUDIO_BUFFER_LEN - (I_buffer_write - I_buffer_read);
    } else {
        return I_buffer_read - I_buffer_write;
    }
}

audio_file_result start_sd_audio_read() {
    // Run the DMA

    // Start reading audio data from SD card into buffer via DMA
    // Triggered when buffer falls below LOAD_WHEN threshold.
    
}

void stop_sd_audio_read() {
    // Stop the DMA
    
    // Stop reading audio data from SD card
    // Triggered when buffer is full or EOF reached.

}

//////////////////////
//  BUFFER -> PWM   //
//////////////////////

uint8_t configure_audio_dma () {
    // Initialize DMA to copy from spi to audio buffer
}

void start_audio_playback() {
    // Start audio playback using the audio buffer
    // Let freerun, will play audio as long as buffer_avail > 0
}

void stop_audio_playback() {

}