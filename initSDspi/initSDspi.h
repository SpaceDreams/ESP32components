#pragma once
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#define SPI_DMA_CHAN        SPI_DMA_CH_AUTO
#define SD_MOUNT_POINT      "/sdcard"
#define PIN_NUM_MISO        CONFIG_INIT_SPI_MISO_GPIO
#define PIN_NUM_MOSI        CONFIG_INIT_SPI_MOSI_GPIO
#define PIN_NUM_CLK         CONFIG_INIT_SPI_SCLK_GPIO
#define PIN_NUM_CS          CONFIG_INIT_SPI_CS_GPIO

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