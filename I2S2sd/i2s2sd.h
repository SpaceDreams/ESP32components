/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once
/* I2S Digital Microphone Recording Example */
#include "init_i2s.h"
#include "initSDmmc.h"
#include "format_wav.h"

#define NUM_CHANNELS        (2) // 1 is for mono recording only!
#define SINGLE_SAMPLE_SIZE  (INIT_AUDIO_BIT_WIDTH / 8 ) + 1 // I can store in 2 bytes, or 4 bytes but not 3 bytes.
#define SAMPLE_SIZE         (SINGLE_SAMPLE_SIZE * 1024)
#define BYTE_RATE           ((INIT_AUDIO_SAMPLE_RATE * (INIT_AUDIO_BIT_WIDTH/8)) * NUM_CHANNELS)

#ifdef __cplusplus
extern "C" {
#endif
void record_wav(uint32_t rec_time, const char *filename);
#ifdef __cplusplus
}
#endif
