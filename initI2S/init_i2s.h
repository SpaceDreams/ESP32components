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
#define INIT_I2S_SLOT_BIT_WIDTH      32 // This is the size of the slot
#define INIT_I2S_SLOT_NUMS           1

/* ESP32 configuration */
#define INIT_CODEC_MCLK_MULTIPLE     (INIT_AUDIO_BIT_WIDTH == 24 ? 384 : 256) 
#define INIT_CODEC_MCLK_FREQ_HZ      (INIT_CODEC_MCLK_MULTIPLE * INIT_AUDIO_SAMPLE_RATE)
#define INIT_POLLING_CYCLE           100 //in milliseconds, must be an integer. This is time required to process all slots of one sample

/*BUFFER CONFIG BOUNDARIES*/
#define dma_buffer_size_max          4092
#define dma_frame_num_max            (dma_buffer_size_max/INIT_I2S_SLOT_NUMS/INIT_AUDIO_BIT_WIDTH*8)
#define interrupt_interval_max       (dma_frame_num_max  / (INIT_AUDIO_SAMPLE_RATE/1000) )
                                     // Careful with units, Here I want polling cycle and interrupt interval to have the same units
#define dma_desc_num_min             ((INIT_POLLING_CYCLE / interrupt_interval_max) + 1) //adding one is for the ceiling function
#define recv_buffer_size_min         dma_desc_num_min * dma_buffer_size_max

/* I2S DMA configuration */
#define INIT_I2S_DMA_DESC_NUM        dma_desc_num_min
#define INIT_I2S_DMA_FRAME_NUM       (INIT_AUDIO_BIT_WIDTH == 24 ? (dma_frame_num_max/3)*3 : dma_frame_num_max) 
                                     // Needs to be a multiple of 3 for a 24 bit adc
#define INIT_SAMPLE_SIZE            (INIT_AUDIO_BIT_WIDTH == 24 ? (recv_buffer_size_min/3+1)*3 : recv_buffer_size_min)

extern i2s_chan_handle_t rx_handle;

extern void init_microphone(void);
