/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/* I2S Digital Microphone Recording Example */
#include "init_I2S.h"
#include "I2Sloop.h"
#include <string.h> // Needed for memset
#include "freertos/FreeRTOS.h" // need for portMAXDELAY
#include "freertos/task.h" // for portMAXDELAY
//#define SINGLE_SAMPLE_SIZE  (32 / 8)  // I can store in 2 bytes, or 4 bytes but not 3 bytes.
#define SAMPLE_SIZE         INIT_SAMPLE_SIZE //(SINGLE_SAMPLE_SIZE * 1024) // Was 1024 Using a 24 bit 
#define ENABLE_MIC_PIN      14

bool stream_audio(uint8_t* raw_buffer, size_t bytes_read, struct sampleArgs* Args)
{
    for (int i=0;i<bytes_read/3;i++){
        int32_t value = 0;
        for (int j = 0; j<3; j++)
            value |= (int32_t)raw_buffer[i+j]<<(8*j);
        if (value & 0x00800000)
               value |= 0xFF000000;
        printf("%ld\n",value);
    }
    return true;
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
    struct sampleArgs NewArgs;
    NewArgs.loop_callback = stream_audio;
    sample_audio(&NewArgs);
}