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
//polling cycle: time it takes to process the entire DMA Buffer.
//The maximum time required to process the total DMA Buffer (ie 1/INIT_AUDIO_SAMPLE_RATE*num_of_samples_per_DMA_buffer)
#define INIT_POLLING_CYCLE           60 //in milliseconds, must be an integer. 

/*BUFFER CONFIG BOUNDARIES*/
#define dma_buffer_size_max          4092 //bytes
#define dma_frame_num_max            (dma_buffer_size_max/INIT_I2S_SLOT_NUMS/INIT_AUDIO_BIT_WIDTH*8) // For 24 bit: 1364
#define INIT_I2S_DMA_FRAME_NUM       (INIT_AUDIO_BIT_WIDTH == 24 ? (dma_frame_num_max/3)*3 : dma_frame_num_max) // For 24 bit: 1362

#define interrupt_interval_max       (INIT_I2S_DMA_FRAME_NUM  / (INIT_AUDIO_SAMPLE_RATE/1000) ) // For 24 bit: 28
                                     // Careful with units, Here I want polling cycle and interrupt interval to have the same units
#define dma_desc_num_min             ((INIT_POLLING_CYCLE / interrupt_interval_max) + 1) //adding one is for the ceiling function // For 24 bit: 4
#define recv_buffer_size_min         dma_desc_num_min * dma_buffer_size_max // 2*4092 =8184

/* I2S DMA configuration */
#define INIT_I2S_DMA_DESC_NUM        dma_desc_num_min
                                     // Needs to be a multiple of 3 for a 24 bit adc
#define INIT_SAMPLE_SIZE            (INIT_AUDIO_BIT_WIDTH == 24 ? (recv_buffer_size_min/3+1)*3 : recv_buffer_size_min) // For 24 bit: 8187

extern i2s_chan_handle_t rx_handle;

extern void init_microphone(void);
