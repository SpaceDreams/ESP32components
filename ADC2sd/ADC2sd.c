#include "ADC2sd.h"


static const char *ADC2sdTAG = "ADC_2_SD";

bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    return (mustYield == pdTRUE);
}

void record_wav(uint32_t rec_time, const char *filename)
{
    // Mount the SDCard for recording the audio file
    mount_sdcard();
    char filedir[strlen(SD_MOUNT_POINT)+strlen(filename)];
    snprintf(filedir, sizeof(filedir), "%s%s", SD_MOUNT_POINT, filename);
    //SD setup
    // Use POSIX and C standard library functions to work with files.
    ESP_LOGI(ADC2sdTAG, "Opening file");

    int total_samples_remaining = init_ADC_SAMPLING_FREQ * rec_time;
    const wav_header_t wav_header =
        WAV_HEADER_PCM_DEFAULT(init_ADC_SAMPLING_FREQ*rec_time*16/8, 16, CONFIG_INIT_SAMPLE_RATE, 1);

    // First check if file exists before creating a new file.
    struct stat st;
    if (stat(filedir, &st) == 0) {
        // Delete it if it exists
        unlink(filedir);
    }

    // Create new WAV file
    FILE *f = fopen(filedir, "wb");
    if (f == NULL) {
        ESP_LOGE(ADC2sdTAG, "Failed to open file for writing");
        return;
    }

    // Write the header to the WAV file
    fwrite(&wav_header, sizeof(wav_header), 1, f);
    
    //ADC setup
    esp_err_t ret;
    uint32_t ret_num = 0;
    uint8_t result[init_READ_LEN] = {0};
    memset(result, 0xcc, init_READ_LEN);
    adc_continuous_handle_t handle = NULL;
    s_task_handle = xTaskGetCurrentTaskHandle();
    continuous_adc_init(channel, NumOfChannelsUsed / sizeof(adc_channel_t), &handle);
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(handle));

    // Start recording
    while (total_samples_remaining>0) {
        /**
         * This is to show you the way to use the ADC continuous mode driver event callback.
         * This `ulTaskNotifyTake` will block when the data processing in the task is fast.
         * However in this example, the data processing (print) is slow, so you barely block here.
         *
         * Without using this event callback (to notify this task), you can still just call
         * `adc_continuous_read()` here in a loop, with/without a certain block timeout.
         */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // Read the RAW samples from the microphone
        ret = adc_continuous_read(handle, result, init_READ_LEN, &ret_num, 0);
        while((ret==ESP_OK) && (total_samples_remaining>0)){
            adc_continuous_data_t parsed_data[ret_num / SOC_ADC_DIGI_RESULT_BYTES];
            uint32_t num_parsed_samples = 0;
            esp_err_t parse_ret = adc_continuous_parse_data(handle, result, ret_num, parsed_data, &num_parsed_samples);
            if(parse_ret==ESP_OK){
                for (int i = 0; i < num_parsed_samples; i++) {
                        if (parsed_data[i].valid) {
                            // The default main stack size is 3584 Bytes; so in the configmenu I am increasing it to 8192 Bytes
                            // Write 2 byte samples to the WAV file
                            fwrite(&parsed_data[i].raw_data, 2, 1, f);
                            total_samples_remaining -= 1;
                        } else {
                            ESP_LOGW(ADC2sdTAG, "Invalid data [ADC%d_Ch%d_%"PRIu32"]",
                                     parsed_data[i].unit + 1,
                                     parsed_data[i].channel,
                                     parsed_data[i].raw_data);
                        }
                    }
            } else{
                ESP_LOGE(ADC2sdTAG, "Data parsing failed: %s", esp_err_to_name(parse_ret));
            }
        }
        if(ret == ESP_ERR_TIMEOUT){
            ESP_LOGE(ADC2sdTAG, "ADC Timeout Error");
            //We try to read `init_READ_LEN` until API returns timeout, which means there's no available data
            break;
        }
    }
    ESP_LOGI(ADC2sdTAG, "Recording done!");
    fclose(f);
    ESP_LOGI(ADC2sdTAG, "File written on SDCard");
    // All done, Turn off ADC
    ESP_ERROR_CHECK(adc_continuous_stop(handle));
    ESP_ERROR_CHECK(adc_continuous_deinit(handle));
    // All done, unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, card);
    ESP_LOGI(ADC2sdTAG, "Card unmounted");
}