/* I2S Digital Microphone Recording Example */
#include "initI2S_ei.h"
#include "esp_log.h"

uint8_t i2s_readraw_buff[SAMPLE_SIZE];
static const char* TAG = "I2S_TESTS";
void sample_audio(void *ArgPointer)
{
    struct sampleArgs *funcArgs = (struct sampleArgs *) ArgPointer;
    // 1. Reset the pin to its default state
    gpio_reset_pin(ENABLE_MIC_PIN);

    // 2. Set the pin direction to output mode
    gpio_set_direction(ENABLE_MIC_PIN, GPIO_MODE_OUTPUT);

    // 3. Set the pin level to HIGH (1)
    gpio_set_level(ENABLE_MIC_PIN, 1);
    init_microphone();
    size_t bytes_read;
    // According to the documentation data isn't valid for a certain time limit.
    printf("I2S streaming starts now\n--------------------------------------\n");
    bool keep_reading_i2s=true;
    while (keep_reading_i2s) {
        // Read the RAW samples from the microphone
        if (i2s_channel_read(rx_handle, i2s_readraw_buff, SAMPLE_SIZE, &bytes_read, portMAX_DELAY) == ESP_OK) {
            keep_reading_i2s=funcArgs->loop_callback(i2s_readraw_buff, bytes_read, funcArgs->rec_file);
        } else {
            printf("Read Failed!\n");
        }
    }
    ESP_LOGI(TAG, "Completed Recording Shutting down I2S");
    /* Have to stop the channel before deleting it */
    i2s_channel_disable(rx_handle);
    /* If the handle is not needed any more, delete it to release the channel resources */
    i2s_del_channel(rx_handle);
    vTaskDelete(NULL);
}