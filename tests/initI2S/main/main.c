/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/* I2S Digital Microphone Recording Example */
#include "init_I2S.h"
#include <string.h> // Needed for memset
#include "freertos/FreeRTOS.h" // need for portMAXDELAY
#include "freertos/task.h" // for portMAXDELAY
//#define SINGLE_SAMPLE_SIZE  (32 / 8)  // I can store in 2 bytes, or 4 bytes but not 3 bytes.
#define SAMPLE_SIZE         516 //(SINGLE_SAMPLE_SIZE * 1024) // Was 1024 Using a 24 bit 
#define ENABLE_MIC_PIN 14

static uint8_t i2s_readraw_buff[SAMPLE_SIZE];
size_t bytes_read;

void stream_audio()
{
    init_microphone();
    // Track whether the current 3-byte block is Left or Right
    bool is_left_channel = true; 
    while (true) {
        // Read the RAW samples from the microphone
        if (i2s_channel_read(rx_handle, i2s_readraw_buff, SAMPLE_SIZE, &bytes_read, portMAX_DELAY) == ESP_OK) {
            //size_t complete_bytes = bytes_read - (bytes_read % 3);
            for (int i = 0; i < bytes_read; i+=3) {
                int32_t value = 0;
                for (int j = 0; j<3; j++)
                    value |= i2s_readraw_buff[i+j]<<8*j;
                //printf("Raw Integer: %ld | Raw Hex: 0x%08lX\n", value>>8, value>>8);
                if (value & 0x00800000) {
                       value |= 0xFF000000; 
                }
                if(is_left_channel)
                    printf("%ld\n",value);
                    //printf("Raw Integer: %ld | Raw Hex: 0x%08lX\n", value>>8, value>>8);
                is_left_channel = !is_left_channel;
            }
                //printf("%ld \n", i2s_readraw_buff[i]>>8);
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