/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// This example uses SDMMC peripheral to communicate with SD card.

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "initSDmmc.h"

#include "esp_cpu.h" // Required for cycle counting functions
#include "esp_timer.h" // Required for timing function
#include <math.h> // Required for sqrt()

#define BUFFER_SIZE   512   // 512B chunk
#define TOTAL_TESTS  5          // 5 tests * 512B = 2560B total test


static const char *TAG = "Speed_Test";

void app_main(void)
{
    mount_sdcard();

    // Use POSIX and C standard library functions to work with files:

    // First create a file.
    const char *file_hello = SD_MOUNT_POINT"/SDmmcSpeedTest.txt";
    // Check if destination file exists before renaming
    struct stat st1;
    if (stat(file_hello, &st1) == 0) {
        // Delete it if it exists
        unlink(file_hello);
        ESP_LOGE(TAG, "Unlinked file path: %s",file_hello);
    }
    FILE *f = fopen(file_hello, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ;
    }
    uint8_t buf[BUFFER_SIZE];
    memset(buf, 8, sizeof(buf));

    //Count cycle differences and time differences required for raw sector writes
    uint32_t dcycles[TOTAL_TESTS];
    uint64_t dtime[TOTAL_TESTS];
    dcycles[0]=esp_cpu_get_cycle_count();
    dtime[0]=esp_timer_get_time();
    for (int i = 0; i < TOTAL_TESTS; i += 1) {
        fwrite(&buf, sizeof(uint8_t), sizeof(buf) , f);
        dcycles[i+1]=esp_cpu_get_cycle_count();
        dtime[i+1]=esp_timer_get_time();
        dcycles[i]=dcycles[i+1]-dcycles[i];
        dtime[i]=dtime[i+1]-dtime[i];
    }
    // All done, close file, unmount partition and disable SDMMC peripheral
    fclose(f);
    esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, card);
    ESP_LOGI(TAG, "Card unmounted");

    // Calculate Statistics: Note: I will divide by the total samples last to maintain accuracy
    //Average:
    uint32_t sumof_cycles = 0;
    uint64_t sumof_time = 0;
    for (int i = 0; i < TOTAL_TESTS; i += 1){
        sumof_cycles += dcycles[i];
        sumof_time += dtime[i];
    }
    //Variance:
    uint32_t unnorm_var_cycles = 0;
    uint64_t unnorm_var_time = 0;
    for (int i = 1; i <= TOTAL_TESTS; i += 1){
        unnorm_var_cycles += pow(TOTAL_TESTS*dcycles[i]-sumof_cycles,2);
        unnorm_var_time += pow(TOTAL_TESTS*dtime[i]-sumof_time,2);
    }

    printf("Writing 512 Bytes took: %f +/- %f clock cycles\n", 
        (float)sumof_cycles/TOTAL_TESTS,sqrt(unnorm_var_cycles)/sqrt(TOTAL_TESTS)/TOTAL_TESTS);
    printf("Writing 512 Bytes took: %lf +/- %lf micro-seconds \n",
        (double)sumof_time/TOTAL_TESTS,sqrt(unnorm_var_time)/sqrt(TOTAL_TESTS)/TOTAL_TESTS);
}
