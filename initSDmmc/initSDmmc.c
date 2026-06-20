#include "initSDmmc.h"

const char SDTAG[] = "init_SD";

// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.
sdmmc_host_t host = SDMMC_HOST_DEFAULT();
sdmmc_card_t *card;

#if CONFIG_EXAMPLE_PIN_CARD_POWER_RESET
static esp_err_t s_example_reset_card_power(void)
{
    esp_err_t ret = ESP_FAIL;
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL<<CONFIG_EXAMPLE_PIN_CARD_POWER_RESET),
    };
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(SDTAG, "Failed to config GPIO");
        return ret;
    }

    ret = gpio_set_level(CONFIG_EXAMPLE_PIN_CARD_POWER_RESET, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(SDTAG, "Failed to set GPIO level");
        return ret;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);

    ret = gpio_set_level(CONFIG_EXAMPLE_PIN_CARD_POWER_RESET, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(SDTAG, "Failed to set GPIO level");
        return ret;
    }

    return ESP_OK;
}
#endif // CONFIG_EXAMPLE_PIN_CARD_POWER_RESET

void mount_sdcard(void)
{
    esp_err_t ret;
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    const char mount_point[] = SD_MOUNT_POINT;
    ESP_LOGI(SDTAG, "Initializing SD card");
    ESP_LOGI(SDTAG, "Using SDMMC peripheral");
    host.unaligned_multi_block_rw_max_chunk_size = 8;
    #if CONFIG_EXAMPLE_SDMMC_SPEED_HS
        host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
    #elif CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_SDR50
        host.slot = SDMMC_HOST_SLOT_0;
        host.max_freq_khz = SDMMC_FREQ_SDR50;
        host.flags &= ~SDMMC_HOST_FLAG_DDR;
    #elif CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_DDR50
        host.slot = SDMMC_HOST_SLOT_0;
        host.max_freq_khz = SDMMC_FREQ_DDR50;
    #elif CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_SDR104
        host.slot = SDMMC_HOST_SLOT_0;
        host.max_freq_khz = SDMMC_FREQ_SDR104;
        host.flags &= ~SDMMC_HOST_FLAG_DDR;
    #endif
  
    #if CONFIG_EXAMPLE_PIN_CARD_POWER_RESET
        ESP_ERROR_CHECK(s_example_reset_card_power());
    #endif
    
        // This initializes the slot without card detect (CD) and write protect (WP) signals.
        // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
        sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    #if EXAMPLE_IS_UHS1
        slot_config.flags |= SDMMC_SLOT_FLAG_UHS1;
    #endif
    
        // Set bus width to use:
    #ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
        slot_config.width = 4;
    #else
        slot_config.width = 1;
    #endif
    
        // On chips where the GPIOs used for SD card can be configured, set them in
        // the slot_config structure:
    #ifdef CONFIG_SOC_SDMMC_USE_GPIO_MATRIX
        slot_config.clk = CONFIG_EXAMPLE_PIN_CLK;
        slot_config.cmd = CONFIG_EXAMPLE_PIN_CMD;
        slot_config.d0 = CONFIG_EXAMPLE_PIN_D0;
        #ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
            slot_config.d1 = CONFIG_EXAMPLE_PIN_D1;
            slot_config.d2 = CONFIG_EXAMPLE_PIN_D2;
            slot_config.d3 = CONFIG_EXAMPLE_PIN_D3;
        #endif  // CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
    #endif  // CONFIG_SOC_SDMMC_USE_GPIO_MATRIX
    
    // Enable internal pullups on enabled pins. The internal pullups
    // are insufficient however, please make sure 10k external pullups are
    // connected on the bus. This is for debug / example purpose only.
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ESP_LOGI(SDTAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(SDTAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(SDTAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
            #ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
                check_sd_card_pins(&config, pin_count);
            #endif
        }
    return;
    }
    ESP_LOGI(SDTAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
}