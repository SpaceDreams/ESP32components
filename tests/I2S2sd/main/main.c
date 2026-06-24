#include "I2S2SD.h"


static const char *TAG = "I2S_2_SD_testing";
#define ENABLE_MIC_PIN 14
void app_main(void)
{
    // 1. Reset the pin to its default state
    gpio_reset_pin(ENABLE_MIC_PIN);

    // 2. Set the pin direction to output mode
    gpio_set_direction(ENABLE_MIC_PIN, GPIO_MODE_OUTPUT);

    // 3. Set the pin level to HIGH (1)
    gpio_set_level(ENABLE_MIC_PIN, 1);
    // According to the documentation data isn't valid 
    printf("I2S recording example start\n--------------------------------------\n");
    ESP_LOGI(TAG, "Starting the recording for %d seconds!", CONFIG_REC_TIME);
    const char *filename = "I2S_record.wav";
    // Start Recording
    record_wav(CONFIG_REC_TIME, filename);
}