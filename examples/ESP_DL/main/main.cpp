/* Include ----------------------------------------------------------------- */

#include "main.hpp"


static const char *TAG = "ESP-DL Testing";

extern "C" bool audio_callback(uint8_t* raw_buffer, size_t n_bytes, struct sampleArgs* Args)
{  // There is a catch here; i2s is 24 bit; so 
    bool keep_reading_i2s = true;
    for (int i = 0; i < n_bytes/3; i++) {
        uint32_t packed_data = 0;
        for (int j = 0; j<3; j++)
            packed_data |= (uint32_t)raw_buffer[j]<<(8*j);
        // Perform Sign Extension (Crucial for negative sound wave numbers)
        // A 24-bit signed number has its sign bit at bit position 23.
        // If bit 23 is a 1, the number is negative, and we must fill the top 8 bits with 1s.
        if (packed_data & 0x00800000)
                packed_data |= 0xFF000000; // Force top byte to be negative padding
        int32_t res = static_cast<int32_t>(packed_data);//Cast to a signed int first then float
        streameddata.buffers[streameddata.buf_select][streameddata.buf_count] = static_cast<float>(res);
        streameddata.buf_count++;
        if(streameddata.buf_count >= streameddata.n_samples) {
            streameddata.buf_select ^= 1;
            streameddata.buf_count = 0;
            // Safely increment by 1 across any CPU core
            atomic_fetch_add(&streameddata.swapped_buffers_count, 1);
            xSemaphoreGive(streameddata.buf_ready);
            if(streameddata.CompletedSaving){
                keep_reading_i2s = false;
                xSemaphoreGive(ShutDownI2S);
            }
        }
    }
    // Save here
    if(!streameddata.CompletedSaving){
        FILE* f = Args->rec_file;
        size_t dumvar = n_bytes;
        if((streameddata.rec_samples+dumvar/3) > TOTSAMPLES){
            dumvar = (TOTSAMPLES - streameddata.rec_samples)*3;
            streameddata.CompletedSaving = true;// Done saving
        }
        fwrite(raw_buffer, sizeof(raw_buffer[0]), dumvar, f);
        streameddata.rec_samples += dumvar/3;
        if(streameddata.CompletedSaving){
            fclose(f);
            ESP_LOGI(TAG, "I2S Data Saved; file closed.");
        }
    }
    return keep_reading_i2s;
}

bool microphone_start(uint32_t n_samples, FILE* f)
{   
    memset(streameddata.buffers, 0, sizeof(streameddata.buffers));
    streameddata.rec_samples = 0;
    streameddata.buf_select = 0;
    streameddata.buf_count = 0;
    streameddata.n_samples = n_samples;
    streameddata.CompletedSaving = false;
    streameddata.buf_ready = xSemaphoreCreateBinary();
    ShutDownI2S = xSemaphoreCreateBinary();
    if (streameddata.buf_ready == NULL || ShutDownI2S == NULL ) {
        ESP_LOGE(TAG, "\nFailed to create semaphore!");
        return false;
    }
    //streameddata.swapped_buffers_count = 0;
    ESP_LOGI(TAG, "Recording is Starting");
    static struct sampleArgs myArgs = {
        .loop_callback = audio_callback,
        .rec_file = f
         };
    xTaskCreatePinnedToCore(
        SampleAudioTask,            // Task function
        "Sample_I2S_data",       // Task name
        10000,                 // Max Dyanmic Bytes required for task, static bytes are pre-allocated
        &myArgs,              // Pointer to your struct of arguments
        1,                    // Task priority
        NULL,                 // Task handle
        1                    // Pin to Core 1 (APP_CPU)
    );

    return true;
}

nvs_handle_t get_counter(int32_t *counter_pointer){
    /////// Determine Number of Total Recordings since last flashed.
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    // Open NVS handle
    ESP_LOGI(TAG, "\nOpening Non-Volatile Storage (NVS) handle...");
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return 255; //This would be an error
    }
    // Read back the value
    ESP_LOGI(TAG, "\nReading counter from NVS...");
    err = nvs_get_i32(my_handle, "counter", counter_pointer);
    switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG, "Read counter = %" PRIu32, *counter_pointer);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGW(TAG, "The value is not initialized yet!");
            *counter_pointer = 0;
            break;
        default:
            ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
    }
    return my_handle;
}

extern "C" void app_main(){
    // 1. Reset the pin to its default state
    gpio_reset_pin(ENABLE_MIC_PIN);

    // 2. Set the pin direction to output mode
    gpio_set_direction(ENABLE_MIC_PIN, GPIO_MODE_OUTPUT);

    // 3. Set the pin level to HIGH (1)
    gpio_set_level(ENABLE_MIC_PIN, 1);

    // This is for setting up multiple cases:
    int32_t boot_counter = 0;
    nvs_handle_t my_handle = get_counter(&boot_counter);
    boot_counter = boot_counter%3;//This is the number of cases; this way I don't have to reset the flash after 3 boots
    const char *classification_cases[] = {"faucet_off","faucet_on","faucet_onoff"};
    int length = snprintf(NULL, 0, "faucetfile_16bit_%s_espdl.wav", classification_cases[boot_counter]);
    char *wavfilename = (char *)malloc((length + 1)* sizeof(char));
    snprintf(wavfilename, length + 1, "faucetfile_24bit_%s_espdl.wav", classification_cases[boot_counter]);

    // Here I am opening the file and creating the name:
    mount_sdcard();
    const char *mount_point = SD_MOUNT_POINT;
    FILE* rec_file = init_wavfile(REC_TIME, wavfilename);
    // summary of inferencing settings (from model_metadata.h)
    printf("Transform settings:\n");
    printf("\tInterval: ");
    printf_float((float)WINDOWSAMPLES/(INIT_AUDIO_SAMPLE_RATE/1000));
    printf(" ms.\n");
    printf("\tStride: %d ms.\n", WINDOWSTRIDE / (INIT_AUDIO_SAMPLE_RATE/1000));

    // Here I am creating variables for saving the classifications:
    float classification_results[TOT_CLASSIFICATIONS][CLASSIFIER_LABEL_COUNT];
    uint32_t curr_classifications = 0;
    const char* const class_labels[] = CLASSIFIER_LABELS;
    uint32_t curr_samples =0;
    // Now I can initalize the classifier
    if (run_classifier_init() == false) {
        ESP_LOGE(TAG, "Issue Initializing Model");
        return;
    }
    // Now I can start the microphone and start recording
    if (microphone_start(WINDOWSTRIDE,rec_file) == false) {
        ESP_LOGE(TAG, "Issue starting microphone");
        return;
    }
    // now I can loop through everything and it's the same
    while(curr_samples<TOTSAMPLES)
    {
        if (xSemaphoreTake(streameddata.buf_ready, portMAX_DELAY) != pdTRUE) continue;
        uint32_t total_events = atomic_exchange(&streameddata.swapped_buffers_count, 0);
        if (total_events > 1)
            ESP_LOGE(TAG, "Data missed! Buffer was swapped %d times before reading!", total_events);
        run_classifier_continuous(streameddata.buffers[streameddata.buf_select^1], classification_results[curr_classifications]);
        curr_samples += WINDOWSTRIDE;
        if (curr_samples<WINDOWSAMPLES) continue;
        curr_classifications++;
    }
    ESP_LOGI(TAG, "Completed Modeling");
    if(xSemaphoreTake(ShutDownI2S, portMAX_DELAY) != pdTRUE) ESP_LOGI(TAG, "Took too long to finish Shutting Down I2S");
    // Create the filename for the predictions
    length = snprintf(NULL, 0, "inference_logs_16bit_%s_espdl.txt", classification_cases[boot_counter]);
    char *inferencefilename = (char *)malloc((length + 1)* sizeof(char));
    snprintf(inferencefilename, length + 1, "inference_logs_24bit_%s_espdl.txt", classification_cases[boot_counter]);
    FILE* inference_logs = init_file(inferencefilename);

    // save the predictions
    uint16_t classifiertotals[TOT_CLASSIFICATIONS]={0};
    for (int i = 0; i<TOT_CLASSIFICATIONS; i++)
    {
        fprintf(inference_logs,"at %ld ms.:\nPredictions: \n",
                (uint32_t)(i*WINDOWSTRIDE + WINDOWSAMPLES)/(INIT_AUDIO_SAMPLE_RATE/1000));
        for (size_t ix = 0; ix < CLASSIFIER_LABEL_COUNT; ix++) {
            fprintf(inference_logs, "    %s: %f\n", class_labels[ix], classification_results[i][ix]);
            if(classification_results[i][ix]>0.8) classifiertotals[ix]++;
        }
    }
    // Save the totals:
    for (size_t ix = 0; ix < CLASSIFIER_LABEL_COUNT; ix++)
                fprintf(inference_logs, "Total Classification Probabilities above 0.8    %s: %d\n", 
                        class_labels[ix], classifiertotals[ix]);
    // Done Saving Predictions
    fclose(inference_logs);
    ESP_LOGI(TAG, "Classification Data Saved");
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card Unmounted");

    // Increment the counter then save
    boot_counter++;
    ESP_LOGI(TAG, "\nWriting counter to NVS...");
    esp_err_t err = nvs_set_i32(my_handle, "counter", boot_counter);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write counter!");
    }
}