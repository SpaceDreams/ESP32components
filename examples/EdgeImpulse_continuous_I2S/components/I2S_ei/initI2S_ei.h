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
#define SAMPLE_SIZE         INIT_SAMPLE_SIZE //For 24 bit: 8187 Bytes 
#define ENABLE_MIC_PIN 14

#ifdef __cplusplus
extern "C" {
#endif

struct sampleArgs {
    void (*loop_callback)(uint8_t* raw_buffer, size_t bytes_read);
};

extern uint8_t i2s_readraw_buff[];
extern size_t bytes_read;
void sample_audio(void *ArgPointer);

#ifdef __cplusplus
}
#endif