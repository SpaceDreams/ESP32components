#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "SDcard.h"


#define TEST_MAX_CHAR_SIZE    64

#if CONFIG_SD_CARD_MMC
#define FILENAME "ESP32S3mmc"
#elif CONFIG_SD_CARD_SPI
#define FILENAME "SmartThingsSPI"
#endif
static const char *TAG = "SDtest";

void app_main(void)
{
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.

    ESP_LOGI(TAG, "Initializing SD card");
    mount_sdcard();
    char data[TEST_MAX_CHAR_SIZE];
    snprintf(data, TEST_MAX_CHAR_SIZE, "%s %s!\n", "Hello", card->cid.name);
    FILE * f = init_file(FILENAME"hello.txt");
    fprintf(f, data);
    fclose(f);
    WAVstruct Args={.rec_time=1,.sample_rate=48000,.bit_width=24,.num_channels=1};
    FILE * fwav = init_wavfile(Args,"hellowav.wav");
    fprintf(fwav, data);
    fclose(fwav);
    unmount_sdcard();
}
