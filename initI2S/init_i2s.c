/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/* I2S Digital Microphone Recording Example */
#include "init_i2s.h"


static const char *TAG = "pdm_rec_example";

i2s_chan_handle_t rx_handle = NULL;

void init_microphone(void)
{
#if SOC_I2S_SUPPORTS_PDM2PCM
    ESP_LOGI(TAG, "Receive PDM microphone data in PCM format");
#else
    ESP_LOGI(TAG, "Receive PDM microphone data in raw PDM format");
#endif  // SOC_I2S_SUPPORTS_PDM2PCM
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));

    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(CONFIG_EXAMPLE_SAMPLE_RATE),
        /* The default mono slot is the left slot (whose 'select pin' of the PDM microphone is pulled down) */
#if SOC_I2S_SUPPORTS_PDM2PCM
        .slot_cfg = I2S_PDM_RX_SLOT_PCM_FMT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
#else
        .slot_cfg = I2S_PDM_RX_SLOT_RAW_FMT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
#endif
        .gpio_cfg = {
            .clk = CONFIG_EXAMPLE_I2S_CLK_GPIO,
            .din = CONFIG_EXAMPLE_I2S_DATA_GPIO,
            .invert_flags = {
                .clk_inv = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_handle, &pdm_rx_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
}
