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
/**
 * @defgroup I2S_pcm
 * 
 * @brief Reads data from the I2S input and saves to memory
 * 
 * @details This function is used within the reading loop of the I2S function; it can be 
 *         redefined but this is the basic structure and it must take less than the pooling
 *         time to complete whose default is 100ms.
 * 
 *         @includedoc ./I2S_pcm/README.md
 * @{
 */
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

/** @} */ // This closes the SDcard group
#ifdef __cplusplus
}
#endif