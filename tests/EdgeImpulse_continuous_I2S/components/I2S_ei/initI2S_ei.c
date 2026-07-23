/* I2S Digital Microphone Recording Example */
#include "initI2S_ei.h"


uint8_t i2s_readraw_buff[SAMPLE_SIZE];
size_t bytes_read;

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
    // According to the documentation data isn't valid for a certain time limit.
    printf("I2S streaming starts now\n--------------------------------------\n");
    while (true) {
        // Read the RAW samples from the microphone
        if (i2s_channel_read(rx_handle, i2s_readraw_buff, SAMPLE_SIZE, &bytes_read, portMAX_DELAY) == ESP_OK) {
            funcArgs->loop_callback(i2s_readraw_buff, bytes_read);
        } else {
            printf("Read Failed!\n");
        }
    }
    /* Have to stop the channel before deleting it */
    i2s_channel_disable(rx_handle);
    /* If the handle is not needed any more, delete it to release the channel resources */
    i2s_del_channel(rx_handle);
    vTaskDelete(NULL);
}