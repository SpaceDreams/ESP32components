/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/* I2S Digital Microphone Recording Example */
#include "i2s2sd.h"

static const char *i2s_TAG = "pdm_rec_example";

static int8_t i2s_readraw_buff[SAMPLE_SIZE];
size_t bytes_read;
const int WAVE_HEADER_SIZE = 44;

void record_wav(uint32_t rec_time, const char *filename)
{
    mount_sdcard();
    init_microphone();
    // Use POSIX and C standard library functions to work with files.
    int written_samples = 0;
    ESP_LOGI(i2s_TAG, "Opening file");

    uint32_t flash_samples = INIT_AUDIO_SAMPLE_RATE * rec_time;
    const wav_header_t wav_header =
        WAV_HEADER_PCM_DEFAULT(BYTE_RATE * rec_time, 16, INIT_AUDIO_SAMPLE_RATE, 1);
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
    size_t dum_samples;
    int samples_read = 0;
    // Write the header to the WAV file
    fwrite(&wav_header, sizeof(wav_header), 1, f);
    // Start recording
    while (written_samples < flash_samples) {
        // Read the RAW samples from the microphone
        if (i2s_channel_read(rx_handle, i2s_readraw_buff, SAMPLE_SIZE, &bytes_read, 1000) == ESP_OK) {
            samples_read = bytes_read/(INIT_I2S_SLOT_BIT_WIDTH/8);
            dum_samples = ((written_samples + samples_read) < flash_samples) ? samples_read : (flash_samples - written_samples) ;
            for (int i = 0; i < dum_samples; i=i+INIT_I2S_SLOT_BIT_WIDTH/8) {
                uint32_t dumval = i2s_readraw_buff[i]<<24 | i2s_readraw_buff[i+1]<<16 | i2s_readraw_buff[i+2]<<8;
                int32_t signeddumval = ((int32_t)dumval)>>8;
                fwrite(&signeddumval, sizeof(int32_t), 1, f);
                written_samples += 1;
                /*if (!((i2s_readraw_buff[i+3] == 0xFF) | (i2s_readraw_buff[i+3] == 0))) {
                    printf(", Error, last byte is: ");
                    for (int j=0; j<8; j++){
                        printf("%d", (i2s_readraw_buff[i+3] >> (7 - j)) & 1);
                    }
                }*/
            }
        } else {
            printf("Read Failed!\n");
        }
    }

    ESP_LOGI(i2s_TAG, "Recording done!");
    fclose(f);
    ESP_LOGI(i2s_TAG, "File written on SDCard");

    // All done, unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, card);
    ESP_LOGI(i2s_TAG, "Card unmounted");
    #if CONFIG_SPI
    // Deinitialize the bus after all devices are removed
    spi_bus_free(host.slot);
    #endif
    /* Have to stop the channel before deleting it */
    i2s_channel_disable(rx_handle);
    /* If the handle is not needed any more, delete it to release the channel resources */
    i2s_del_channel(rx_handle);
}
