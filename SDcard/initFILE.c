#include "SDcard.h"
#define WAVE_HEADER_SIZE 44
static const char *TAG = "INIT_FILE";

FILE* init_file(const char *filename)
{
    // Use POSIX and C standard library functions to work with files.
    ESP_LOGI(TAG, "Opening file");
    size_t namesize = (strlen(SD_MOUNT_POINT)+1+strlen(filename)+1);
    char *filepath = (char *)malloc(namesize*sizeof(char));
    if (filepath == NULL) return NULL;
    snprintf(filepath, namesize, "%s/%s", SD_MOUNT_POINT, filename);
    // First check if file exists before creating a new file.
    struct stat st;
    if (stat(filepath, &st) == 0) {
        // Delete it if it exists
        unlink(filepath);
        ESP_LOGE(TAG, "%s already exists now deleting",filepath);
    }

    // Create new WAV file
    FILE *f = fopen(filepath, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return NULL;
    }
    return f;
}

FILE* init_wavfile(const WAVstruct Args, const char *filename)
{
    size_t namesize = (strlen(SD_MOUNT_POINT)+1+strlen(filename)+1);
    char *filepath = (char *)malloc(namesize*sizeof(char));
    if (filepath == NULL) return NULL;
    snprintf(filepath, namesize, "%s/%s", SD_MOUNT_POINT, filename);

    // 1. Pre-allocate the entire estimated size (e.g., 10 MB total)
    uint32_t byte_rate=  Args.sample_rate * (Args.bit_width/8) * Args.num_channels;
    uint32_t total_file_size = Args.rec_time*Args.sample_rate*(Args.bit_width/8)+WAVE_HEADER_SIZE;

    // preallocation
    esp_err_t ret = esp_vfs_fat_create_contiguous_file(
        SD_MOUNT_POINT,               // base_path (where your partition/SD card is mounted)
        filepath, // full_path to the file
        total_file_size,        // size in bytes (e.g., 10 MB)
        true                     // alloc_now (true = mode 1, false = mode 0)
    );
    FILE* f = init_file(filename);
    const wav_header_t wav_header =
        WAV_HEADER_PCM_DEFAULT(byte_rate * Args.rec_time, Args.bit_width, Args.sample_rate, Args.num_channels);
    // Write the header to the WAV file
    fwrite(&wav_header, WAVE_HEADER_SIZE, 1, f);
    return f;
}

bool rename_sdcard_file(const char *old_filename,const char *new_filename){
    size_t namesize = (strlen(SD_MOUNT_POINT)+1+strlen(old_filename)+1);
    char *old_filepath = (char *)malloc(namesize*sizeof(char));
    if (old_filepath == NULL) return NULL;
    snprintf(old_filepath, namesize, "%s/%s", SD_MOUNT_POINT, old_filename);

    namesize = (strlen(SD_MOUNT_POINT)+1+strlen(new_filename)+1);
    char *new_filepath = (char *)malloc(namesize*sizeof(char));
    if (new_filepath == NULL) return NULL;
    snprintf(new_filepath, namesize, "%s/%s", SD_MOUNT_POINT, new_filename);

    if (rename(old_filepath, new_filepath) == 0)
            ESP_LOGI(TAG,"File successfully renamed to %s", new_filepath);
    else 
        ESP_LOGE(TAG,"Error renaming file");
    return true;
}