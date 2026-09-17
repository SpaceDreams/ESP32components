/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/* I2S Digital Microphone Recording Example */
#include "SDcard.h"
//#include "format_wav.h" <- this can be used later after testing

static const char *sd_TAG = "INIT_FILE";
const int WAVE_HEADER_SIZE = 44;

//mount_sdcard();

FILE* init_file(const char *filename)
{
    // Use POSIX and C standard library functions to work with files.
    ESP_LOGI(sd_TAG, "Opening file");
    size_t namesize = (strlen(SD_MOUNT_POINT)+1+strlen(filename)+1);
    char *filepath = malloc(namesize*sizeof(char));
    if (filepath == NULL) return NULL;
    snprintf(filepath, namesize, "%s/%s", SD_MOUNT_POINT, filename);
    // First check if file exists before creating a new file.
    struct stat st;
    if (stat(filepath, &st) == 0) {
        // Delete it if it exists
        unlink(filepath);
        ESP_LOGE(sd_TAG, "%s already exists now deleting",filepath);
    }

    // Create new WAV file
    FILE *f = fopen(filepath, "wb");
    if (f == NULL) {
        ESP_LOGE(sd_TAG, "Failed to open file for writing");
        return NULL;
    }
    return f;
}

FILE* init_wavfile(const WAVstruct Args, const char *filename)
{
    FILE* f = init_file(filename);
    
    // 1. Pre-allocate the entire estimated size (e.g., 10 MB total)
    uint32_t byte_rate=  Args.sample_rate * (Args.bit_width/8) * Args.num_channels;
    uint32_t total_file_size = Args.rec_time*Args.sample_rate*(Args.bit_width/8)+WAVE_HEADER_SIZE;
    fseek(f, total_file_size - 1, SEEK_SET);
    fputc(0, f);
    
    // 2. Rewind to the beginning
    fseek(f, 0, SEEK_SET);
    
    const wav_header_t wav_header =
        WAV_HEADER_PCM_DEFAULT(byte_rate * Args.rec_time, Args.bit_width, Args.sample_rate, Args.num_channels);
    // Write the header to the WAV file
    fwrite(&wav_header, sizeof(wav_header), 1, f);
    return f;
}