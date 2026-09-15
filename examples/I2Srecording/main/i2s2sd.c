/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/* I2S Digital Microphone Recording Example */
#include "i2s2sd.h"

#define WAVE_HEADER_SIZE 44
#define rec_time         60
#define filename         "SavingAudioSep15_2026.wav"
#define flash_samples    INIT_AUDIO_SAMPLE_RATE * rec_time * NUM_CHANNELS
#define ENABLE_MIC_PIN 14

static const char *i2s_TAG = "rec_example";

uint32_t written_samples =0;
bool save_audio(uint8_t* raw_buffer, size_t bytes_read, struct sampleArgs* Args)
{
    bool continuesaving=true;
    if (bytes_read <= 0) {
        ESP_LOGE(i2s_TAG,"Error in I2S read : %d", bytes_read);
    }
    else {
        if (bytes_read < SAMPLE_SIZE) ESP_LOGE(i2s_TAG,"Partial I2S read");
        int samples_read = bytes_read/3;
        uint32_t dum_samples = ((written_samples + samples_read) < flash_samples) ? samples_read : (flash_samples - written_samples) ;
        fwrite(raw_buffer, 1, dum_samples*3, Args->rec_file);
        written_samples += dum_samples;
    }
    if(written_samples>=flash_samples)
        continuesaving=false;
    return continuesaving;
}

void app_main(void)
{
    // 1. Reset the pin to its default state
    gpio_reset_pin(ENABLE_MIC_PIN);

    // 2. Set the pin direction to output mode
    gpio_set_direction(ENABLE_MIC_PIN, GPIO_MODE_OUTPUT);

    // 3. Set the pin level to HIGH (1)
    gpio_set_level(ENABLE_MIC_PIN, 1);
    mount_sdcard();
    // Use POSIX and C standard library functions to work with files.
    ESP_LOGI(i2s_TAG, "Opening file");
    const wav_header_t wav_header =
        WAV_HEADER_PCM_DEFAULT(BYTE_RATE * rec_time, INIT_AUDIO_BIT_WIDTH, INIT_AUDIO_SAMPLE_RATE, NUM_CHANNELS);
    char filepath[strlen(SD_MOUNT_POINT)+1+strlen(filename)+1];
    snprintf(filepath, sizeof(filepath), "%s/%s", SD_MOUNT_POINT, filename);
    // First check if file exists before creating a new file.
    struct stat st;
    if (stat(filepath, &st) == 0) {
        // Delete it if it exists
        unlink(filepath);
        ESP_LOGE(i2s_TAG, "%s already exists now deleting",filepath);
    }

    // Create new WAV file
    FILE *f = fopen(filepath, "wb");
    if (f == NULL) {
        ESP_LOGE(i2s_TAG, "Failed to open file for writing");
        return;
    }
    // Write the header to the WAV file
    fwrite(&wav_header, sizeof(wav_header), 1, f);
    struct sampleArgs NewArgs;
    NewArgs.loop_callback = save_audio;
    NewArgs.rec_file = f;
    // Start Recording
    sample_audio(&NewArgs);

    ESP_LOGI(i2s_TAG, "Recording done!");
    fclose(f);
    ESP_LOGI(i2s_TAG, "File written on SDCard");

    // All done, unmount partition and disable SPI peripheral
    unmount_sdcard();
    ESP_LOGI(i2s_TAG, "Card unmounted");
}
