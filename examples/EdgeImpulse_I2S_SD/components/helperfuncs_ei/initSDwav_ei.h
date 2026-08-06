/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once
/* I2S Digital Microphone Recording Example */
#include "format_wav.h"
#include "initSDmmc.h"

#ifdef __cplusplus
extern "C" {
#endif
FILE* init_file(const char *filename);
#ifdef __cplusplus
}
#endif
