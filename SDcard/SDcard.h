#pragma once
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "format_wav.h"

#define SD_MOUNT_POINT      CONFIG_SD_MOUNT_POINT

// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.
extern sdmmc_host_t host;
extern sdmmc_card_t *card;

#ifdef __cplusplus
extern "C" {
#endif

void mount_sdcard(void);
void unmount_sdcard(void);
FILE* init_file(const char *filename);
typedef struct {
    uint32_t rec_time;
    uint32_t sample_rate;
    uint32_t bit_width;
    uint32_t num_channels;
} WAVstruct;
FILE* init_wavfile(const WAVstruct Args, const char *filename);

#ifdef __cplusplus
}
#endif