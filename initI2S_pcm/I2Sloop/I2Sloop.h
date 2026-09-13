/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/* I2S Digital Microphone Recording Example */
#include "init_I2S.h"
#include "freertos/FreeRTOS.h" // need for portMAXDELAY
#include "freertos/task.h" // for portMAXDELAY
#define SAMPLE_SIZE         INIT_SAMPLE_SIZE //For 24 bit: 8187 Bytes 

#ifdef __cplusplus
extern "C" {
#endif

#ifndef I2Sloop_STRUCT
#define I2Sloop_STRUCT

struct sampleArgs {
    bool (*loop_callback)(uint8_t* raw_buffer, size_t bytes_read, struct sampleArgs* Args);
    FILE* rec_file;
};

#endif // MY_STRUCT_H

extern uint8_t i2s_readraw_buff[];
extern size_t bytes_read;
void sample_audio(void *ArgPointer);

#ifdef __cplusplus
}
#endif