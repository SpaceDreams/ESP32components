#pragma once
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"

#define SD_MOUNT_POINT      CONFIG_SD_MOUNT_POINT
//#define SD_MOUNT_POINT      "/sdcard" //Can be defined here or in the configmenu
// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.
extern sdmmc_host_t host;
extern sdmmc_card_t *card;

#ifdef __cplusplus
extern "C" {
#endif

void mount_sdcard(void);

#ifdef __cplusplus
}
#endif