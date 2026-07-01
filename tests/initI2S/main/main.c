/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/* I2S Digital Microphone Recording Example */
#include "init_I2S.h"
#include <string.h> // Needed for memset
#define SINGLE_SAMPLE_SIZE  (32 / 8)  // I can store in 2 bytes, or 4 bytes but not 3 bytes.
#define SAMPLE_SIZE         (SINGLE_SAMPLE_SIZE * 1024)
#define ENABLE_MIC_PIN 14

static uint32_t i2s_readraw_buff[SAMPLE_SIZE];
static uint8_t packed_buffer[SAMPLE_SIZE][4];
size_t bytes_read;

void stream_audio()
{
    init_microphone();
    while (true) {
        // Read the RAW samples from the microphone
        if (i2s_channel_read(rx_handle, i2s_readraw_buff, sizeof(i2s_readraw_buff), &bytes_read, 1000) == ESP_OK) {
            for (int i = 0; i < bytes_read/sizeof(uint32_t); i++) {
                uint8_t *sample_bytes = (uint8_t *)&i2s_readraw_buff[i];
                for (int j=0;j<4;j++)
                    packed_buffer[i][j] = sample_bytes[j]; // Low Audio Byte,Mid Audio Byte,High Audio Byte (MSB)

            }
            for (int i = 0; i < bytes_read/sizeof(uint32_t); i++) {
                if (i%2 == 0){
                    //printf("%d\n",packed_buffer[i][3]);
                    for (int j=0;j<4;j++)
                        printf("%d, ", packed_buffer[i][j]);
                    printf("\n");
                }
                else{
                    printf("right: ");
                    for (int j=0;j<4;j++)
                        printf("%d ", packed_buffer[i][j]);
                    printf("\n");
                }
            }
        } else {
            printf("Read Failed!\n");
        }
    }

    /* Have to stop the channel before deleting it */
    i2s_channel_disable(rx_handle);
    /* If the handle is not needed any more, delete it to release the channel resources */
    i2s_del_channel(rx_handle);
}
void app_main(void)
{
    // 1. Reset the pin to its default state
    gpio_reset_pin(ENABLE_MIC_PIN);

    // 2. Set the pin direction to output mode
    gpio_set_direction(ENABLE_MIC_PIN, GPIO_MODE_OUTPUT);

    // 3. Set the pin level to HIGH (1)
    gpio_set_level(ENABLE_MIC_PIN, 1);
    // According to the documentation data isn't valid for a certain time limit.
    printf("I2S streaming example start\n--------------------------------------\n");
    // Start Recording
    stream_audio();
}