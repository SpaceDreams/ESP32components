/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once
/* I2S Digital Microphone Recording Example */
#include "init_i2s.h"
#include "format_wav.h"
#include "initSDmmc.h"

#define NUM_CHANNELS        (1) // 1 is for mono recording only!
#define SAMPLE_SIZE         INIT_SAMPLE_SIZE
#define BYTE_RATE           ((INIT_AUDIO_SAMPLE_RATE * (INIT_AUDIO_BIT_WIDTH/8)) * NUM_CHANNELS)

extern uint8_t i2s_readraw_buff[];
// 1. Define the callback function blueprint.
// It passes the current loop index, a library-generated value, and user custom data.
//typedef void (*loop_callback)(int samples_read);

struct recordArgs {
	uint32_t rec_time; 
	const char *filename;
    void (*loop_callback)(uint32_t samples_read);
};

#ifdef __cplusplus
extern "C" {
#endif
void record_wav(void *ArgPointer);
#ifdef __cplusplus
}
#endif
