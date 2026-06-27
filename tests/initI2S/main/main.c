/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/* I2S Digital Microphone Recording Example */
#include "init_I2S.h"
#define SINGLE_SAMPLE_SIZE  (32 / 8) // I can store in 2 bytes, or 4 bytes but not 3 bytes.
#define SAMPLE_SIZE         (SINGLE_SAMPLE_SIZE * 1024)
#define ENABLE_MIC_PIN 14

static uint32_t i2s_readraw_buff[SAMPLE_SIZE];
size_t bytes_read;

void stream_audio()
{
    init_microphone();
    while (true) {
        // Read the RAW samples from the microphone
        if (i2s_channel_read(rx_handle, i2s_readraw_buff, sizeof(i2s_readraw_buff), &bytes_read, 1000) == ESP_OK) {
            for (int i = 0; i < bytes_read/sizeof(uint32_t); i=i+2) {
                //uint32_t rawbytes = (i2s_readraw_buff[i]<<24) | (i2s_readraw_buff[i+1]<<16) | (i2s_readraw_buff[i+2]<<8);
                printf("%ld\n", ((long int)i2s_readraw_buff[i])>>8);
                if (!(((i2s_readraw_buff[i]&0xFF) == 0xFF) | ((i2s_readraw_buff[i]&0xFF) == 0))) {
                    printf("Error, last byte is: ");
                    for (int j=0; j<8; j++){
                        printf("%d", (int)((i2s_readraw_buff[i+3] >> (7 - j)) & 1));
                    }
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
    // According to the documentation data isn't valid 
    printf("I2S streaming example start\n--------------------------------------\n");
    // Start Recording
    stream_audio();
}