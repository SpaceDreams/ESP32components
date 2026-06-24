/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once
/* I2S Digital Microphone Recording Example */
#include "driver/i2s_std.h"
#include "driver/gpio.h"

/* I2S port and GPIOs */
#define INIT_I2S_NUM                 (0)
#define INIT_I2S_MCK_IO              CONFIG_I2S_MCK_IO
#define INIT_I2S_BCK_IO              CONFIG_I2S_BCK_IO
#define INIT_I2S_WS_IO               CONFIG_I2S_WS_IO
#define INIT_I2S_DO_IO               CONFIG_I2S_DO_IO
#define INIT_I2S_DI_IO               CONFIG_I2S_DI_IO

/* Audio configuration */
#define INIT_AUDIO_SAMPLE_RATE       CONFIG_AUDIO_SAMPLE_RATE
#define INIT_AUDIO_BIT_WIDTH         CONFIG_AUDIO_BIT_WIDTH

/* ESP32 configuration */
#define INIT_CODEC_MCLK_MULTIPLE     (INIT_AUDIO_BIT_WIDTH == 24 ? 384 : 256)
#define INIT_CODEC_MCLK_FREQ_HZ      (INIT_CODEC_MCLK_MULTIPLE * INIT_AUDIO_SAMPLE_RATE)

/* I2S DMA configuration */
#define INIT_I2S_DMA_DESC_NUM        8
#define INIT_I2S_DMA_FRAME_NUM       96
#define INIT_I2S_DMA_TOTAL_BUFFER_SIZE    (INIT_I2S_DMA_DESC_NUM * (INIT_I2S_DMA_FRAME_NUM * INIT_I2S_DMA_DESC_NUM * INIT_AUDIO_BIT_WIDTH / 8))

extern i2s_chan_handle_t rx_handle;

extern void init_microphone(void);
