#include "SDcard.h"
#include <stdarg.h>
#define WAVE_HEADER_SIZE 44
static const char *TAG = "INIT_FILE";

char * formatted_filename(const char *format, va_list args){
        /**
    * \brief Contructs the full file path name relative to the SD card
    *
    * \details This is a function which handles the concatenation and the creation
    *          of the filename if it came with arguments.
    *
    * \param format Filename which may include Format Placeholders.
    * \param args This is the list of args that go in the placeholders.
    */
    // Measure exactly how many bytes are needed for the filename string
    va_list args_copy;
    va_copy(args_copy, args);
    int length = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    
    if (length < 0) {
        va_end(args);
        ESP_LOGE(TAG, "Error Creating Filename");
        return NULL;
    }

    // 3. Allocate heap memory for the dynamic filename
    char *filename = (char *)malloc(length + 1);
    if (filename == NULL) {
        va_end(args);
        ESP_LOGE(TAG, "Error Allocating Memory");
        return NULL;
    }

    // 4. Perform the actual concatenation into the heap buffer
    vsnprintf(filename, length + 1, format, args);
    va_end(args);
    size_t namesize = (strlen(SD_MOUNT_POINT)+1+strlen(filename)+1);
    char *filepath = (char *)malloc(namesize*sizeof(char));
    if (filepath == NULL) return NULL;
    snprintf(filepath, namesize, "%s/%s", SD_MOUNT_POINT, filename);
    free(filename);
    return filepath;
}

FILE* openANDreturn(const char *filepath)
{
    /**
    * \brief With the filepath given this opens the file.
    *
    * \param filepath This is the filepath
    */
    // Use POSIX and C standard library functions to work with files.
    ESP_LOGI(TAG, "Opening file");
    // Create new file
    FILE *f = fopen(filepath, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return NULL;
    }
    return f;
}

FILE* init_file(const char *format,...)
{
    /**
    * \brief This function initializes a file in the write binary mode.
    *
    * \param format This contains all filename with format placeholders.
    * \param[in] ...    The variable formatting arguments to map to the format specifiers.
    */
    va_list args;
    va_start(args, format); // Capture the "..." into the args variable
    // Pass the format and the packed args list to the worker function
    char * filepath = formatted_filename(format, args);
    // First check if file exists before creating a new file.
    struct stat st;
    if (stat(filepath, &st) == 0) {
        // Delete it if it exists
        unlink(filepath);
        ESP_LOGE(TAG, "%s already exists now deleting",filepath);
    }
    FILE* f = openANDreturn(filepath);
    //Free all malloc's before closing this function
    free(filepath);
    return f;
}

FILE* init_wavfile(const WAVstruct Args, const char *format,...)
{
    /**
    * \brief This function initializes a wav file.
    * 
    * \detail This will create the wav header, preallocate the desired space, 
    *         and write the header to the file.
    *
    * \param format This contains all filename with format placeholders.
    * \param[in] ...    The variable formatting arguments to map to the format specifiers.
    */
    va_list args;
    va_start(args, format); // Capture the "..." into the args variable
    // Pass the format and the packed args list to the worker function
    char * filepath = formatted_filename(format, args);

    // 1. Pre-allocate the entire estimated size (e.g., 10 MB total)
    uint32_t byte_rate=  Args.sample_rate * (Args.bit_width/8) * Args.num_channels;
    uint32_t total_file_size = Args.rec_time*Args.sample_rate*(Args.bit_width/8)+WAVE_HEADER_SIZE;
    // First check if file exists before creating a new file.
    struct stat st;
    if (stat(filepath, &st) == 0) {
        // Delete it if it exists
        unlink(filepath);
        ESP_LOGE(TAG, "%s already exists now deleting",filepath);
    }
    // preallocation
    //esp_err_t ret = esp_vfs_fat_create_contiguous_file(
    esp_vfs_fat_create_contiguous_file(
        SD_MOUNT_POINT,               // base_path (where your partition/SD card is mounted)
        filepath, // full_path to the file
        total_file_size,        // size in bytes (e.g., 10 MB)
        true                     // alloc_now (true = mode 1, false = mode 0)
    );
    FILE* f = openANDreturn(filepath);
    const wav_header_t wav_header =
        WAV_HEADER_PCM_DEFAULT(byte_rate * Args.rec_time, Args.bit_width, Args.sample_rate, Args.num_channels);
    // Write the header to the WAV file
    fwrite(&wav_header, WAVE_HEADER_SIZE, 1, f);
    //Free all malloc's before closing this function
    free(filepath);
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
    free(old_filepath);
    free(new_filepath);
    return true;
}