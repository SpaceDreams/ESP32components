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

static esp_err_t s_test_write_file(const char *path, char *data)
{
    ESP_LOGI(TAG, "Opening file %s", path);
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, data);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    return ESP_OK;
}

static esp_err_t s_test_read_file(const char *path)
{
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[TEST_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    return ESP_OK;
}

void app_main(void)
{
    esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.

    ESP_LOGI(TAG, "Initializing SD card");
    mount_sdcard();
    // Use POSIX and C standard library functions to work with files:
    // First create a file.
    const char *file_hello = SD_MOUNT_POINT"/"FILENAME"hello.txt";
    // Check if destination file exists before renaming
    struct stat st1;
    if (stat(file_hello, &st1) == 0) {
        // Delete it if it exists
        unlink(file_hello);
        ESP_LOGE(TAG, "Unlinked existing hello file");
    }
    char data[TEST_MAX_CHAR_SIZE];
    snprintf(data, TEST_MAX_CHAR_SIZE, "%s %s!\n", "Hello", card->cid.name);
    ret = s_test_write_file(file_hello, data);
    if (ret != ESP_OK) {
        return;
    }

    const char *file_foo = SD_MOUNT_POINT"/"FILENAME"foo.txt";
    // Check if destination file exists before renaming
    struct stat st;
    if (stat(file_foo, &st) == 0) {
        // Delete it if it exists
        unlink(file_foo);
        ESP_LOGE(TAG, "Unlinked existing foo file");
    }

    // Rename original file
    ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
    if (rename(file_hello, file_foo) != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return;
    }

    ret = s_test_read_file(file_foo);
    if (ret != ESP_OK) {
        return;
    }

    const char *file_nihao = SD_MOUNT_POINT"/"FILENAME"nihao.txt";
    memset(data, 0, TEST_MAX_CHAR_SIZE);
    snprintf(data, TEST_MAX_CHAR_SIZE, "%s %s!\n", "Nihao", card->cid.name);
    ret = s_test_write_file(file_nihao, data);
    if (ret != ESP_OK) {
        return;
    }

    //Open file for reading
    ret = s_test_read_file(file_nihao);
    if (ret != ESP_OK) {
        return;
    }

    // All done, unmount partition and disable SDMMC peripheral
    unmount_sdcard();
}
