#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"

#include <hardware/spi.h>
// #include <tf_card.h>

#include "const.h"
#include "sd.h"
#include "sd_audio.h"

uint16_t get_buff_avail() {
    // Get available space in audio buffer
    if (I_buffer_write >= I_buffer_read) {
        return AUDIO_BUFFER_LEN - (I_buffer_write - I_buffer_read);
    } else {
        return I_buffer_read - I_buffer_write;
    }
}

uint8_t open_sd_audio_file(const char* filename) {
    // Close any open audio files first
    close_sd_audio_file();

    // Open the audio file on SD card for reading
    // Store in audio_file global variable
    audio_file = fopen(filename, "rb");
    if (audio_file == NULL) {
        printf("Failed to open audio file: %s\n", filename);
        return 1;
    } else {
        printf("Audio file opened: %s\n", filename);
        return 0;
    }
}

void close_sd_audio_file() {
    // Close the currently open audio file, if any
    if (audio_file == NULL) { return;}
    fclose(audio_file);
    audio_file = NULL;
}

uint8_t start_sd_audio_read() {
    // Run the DMA

    // Start reading audio data from SD card into buffer via DMA
    // Triggered when buffer falls below LOAD_WHEN threshold.
    
}

void stop_sd_audio_read() {
    // Stop the DMA
    
    // Stop reading audio data from SD card
    // Triggered when buffer is full or EOF reached.

}

uint8_t init_audio_dma () {
    // Initialize DMA to copy from spi to audio buffer
}

void start_audio_playback() {
    // Start audio playback using the audio buffer
    // Let freerun, will play audio as long as buffer_avail > 0
}