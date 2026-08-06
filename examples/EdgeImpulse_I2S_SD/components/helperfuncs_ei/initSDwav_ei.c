/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/* I2S Digital Microphone Recording Example */
#include "initSDwav_ei.h"

static const char *sd_TAG = "save2SDwav";
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
