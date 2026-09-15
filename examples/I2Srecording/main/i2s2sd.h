/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once
/* I2S Digital Microphone Recording Example */
#include "I2Sloop.h"
#include "SDcard.h"

#define NUM_CHANNELS        (1) // 1 is for mono recording only!
#define SAMPLE_SIZE         INIT_SAMPLE_SIZE
#define BYTE_RATE           ((INIT_AUDIO_SAMPLE_RATE * (INIT_AUDIO_BIT_WIDTH/8)) * NUM_CHANNELS)
