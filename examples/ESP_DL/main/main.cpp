/* Include ----------------------------------------------------------------- */

#include "main.hpp"

static const char *TAG = "ESP-DL Testing";

extern "C" bool audio_callback(uint8_t* raw_buffer, size_t n_bytes, struct sampleArgs* Args)
{  // There is a catch here; i2s is 24 bit; so 
    bool buf_ready = false;
    bool keep_reading_i2s = true;
    for (int i = 0; i < n_bytes/3; i++) {
        uint32_t packed_data = 0;
        for (int j = 0; j<3; j++)
            packed_data |= (uint32_t)raw_buffer[j]<<(8*j);
        // Perform Sign Extension (Crucial for negative sound wave numbers)
        // A 24-bit signed number has its sign bit at bit position 23.
        // If bit 23 is a 1, the number is negative, and we must fill the top 8 bits with 1s.
        float signfunc = 1;
        if (packed_data & 0x00800000)
                packed_data |= 0xFF000000; // Force top byte to be negative padding
        int32_t res = static_cast<int32_t>(packed_data);//Cast to a signed int first then float
        streameddata.buffers[streameddata.buf_select][streameddata.buf_count] = static_cast<float>(res);
        streameddata.buf_count++;
        if(streameddata.buf_count >= streameddata.n_samples) {
            streameddata.buf_select ^= 1;
            streameddata.buf_count = 0;
            buf_ready = true;
            xSemaphoreGive(streameddata.buf_ready);
            streameddata.swapped_buffers_count++;
        }
    }
    // Save here
    FILE* f = Args->rec_file
    size_t dumvar = n_bytes;
    if(dumvar > totsamples*sizeof(raw_buffer[0])){
        dumvar = (totsamples - rec_samples)*sizeof(raw_buffer[0]);
        keep_reading_i2s = false;// Done Reading I2S
    }
    fwrite(raw_buffer, sizeof(raw_buffer[0]), dumvar, f);
    rec_samples += dumvar/sizeof(raw_buffer[0]);
    // Close file if I do not keep reading I2S
    if(!keep_reading_i2s){
            fclose(f);
            ESP_LOGI(TAG, "I2S Data Saved; file closed.");
            xSemaphoreGive(finishedSaving);
        }
    return keep_reading_i2s;
}

bool microphone_start(uint32_t n_samples, FILE* f)
{
    streameddata.buffers[0] = (float *)calloc(n_samples, sizeof(float));
    streameddata.buffers[1] = (float *)calloc(n_samples, sizeof(float));
    streameddata.buf_select = 0;
    streameddata.buf_count = 0;
    streameddata.n_samples = n_samples;
    //streameddata.buf_ready = 0;
    streameddata.buf_ready = xSemaphoreCreateBinary();
    finishedSaving = xSemaphoreCreateBinary();
    if (streameddata.buf_ready == NULL || finishedSaving == NULL ) {
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
        sample_audio,            // Task function
        "Sample_I2S_data",       // Task name
        10000,                 // Max Bytes required for task // DMA buffer and sample buffer don't count since they were allocated at the program startup
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
    const char *classification_cases[] = {"faucet_off","faucet_on","faucet_onoff"};
    int length = snprintf(NULL, 0, "faucetfile_16bit_%s.wav", classification_cases[boot_counter]);
    char *wavfilename = (char *)malloc((length + 1)* sizeof(char));
    snprintf(wavfilename, length + 1, "faucetfile_16bit_%s.wav", classification_cases[boot_counter]);

    // Here I am opening the file and creating the name:
    const char *mount_point = mount_sdcard();
    FILE* rec_file = init_wavfile(rec_time, wavfilename);
    // summary of inferencing settings (from model_metadata.h)
    printf("Transform settings:\n");
    printf("\tInterval: ");
    printf_float((float)WindowSamples/(INIT_AUDIO_SAMPLE_RATE/1000));
    printf(" ms.\n");
    printf("\tStride: %d ms.\n", WindowStride / (INIT_AUDIO_SAMPLE_RATE/1000));

    // Once the model is created, the input and output memory is allocated.
    dl::Model *model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);

    std::map<std::string, dl::TensorBase *> model_inputs = model->get_inputs();
    dl::TensorBase *model_input = model_inputs.begin()->second;
    std::map<std::string, dl::TensorBase *> model_outputs = model->get_outputs();
    dl::TensorBase *model_output = model_outputs.begin()->second;

    if (microphone_start(WindowStride,rec_file) == false) {
        printf("ERR: Could not allocate audio buffer (size %d), this could be due to the window length of your model\r\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
        return;
    }
    // I have to initalize the transform first, the first transform is a little different than all the rest
    if (xSemaphoreTake(streameddata.buf_ready, portMAX_DELAY) != pdTRUE) continue;
    if (streameddata.swapped_buffers_count > 1)
        ESP_LOGE(TAG, "Data missed! Buffer was swapped %d times before reading!", streameddata.swapped_buffers_count);
    streameddata.swapped_buffers_count = 0; 
    float transformoutput[1][firststrideshape[0]][firststrideshape[1]];
    transform.process(streameddata.buffers[!streameddata.buf_select], WindowStride, transformoutput[0]);
    float buffcopy[WindowStride+overlap]
    memcpy(buffcopy,&streameddata.buffers[!streameddata.buf_select][WindowStride-overlap],
           overlap*sizeof(streameddata.buffers[!streameddata.buf_select][0]));
    dl::TensorBase *transform_tensor = new dl::TensorBase({1, firststrideshape[0],firststrideshape[1]}, nullptr, 0, dl::DATA_TYPE_FLOAT);
    transform_tensor->set_element_ptr(transformoutput);
    dl::TensorBase *transposed_tensor = dl::transpose(transform_tensor, {1,3,2});
    model_tensor->push(transposed_tensor, 1);
    uint32_t curr_samples = WindowStride;
    size_t offset = firststrideshape[0];
    float transformoutput[1][strideshape[0]][strideshape[1]];
    // now I can loop through everything and it's the same
    while(curr_samples<totsamples)
    {
        if (xSemaphoreTake(streameddata.buf_ready, portMAX_DELAY) != pdTRUE) continue;
        if (streameddata.swapped_buffers_count > 1)
            ESP_LOGE(TAG, "Data missed! Buffer was swapped %d times before reading!", streameddata.swapped_buffers_count);
        streameddata.swapped_buffers_count = 0; 
        memcpy(&buffcopy[overlap],&streameddata.buffers[!streameddata.buf_select],
           WindowStride*sizeof(streameddata.buffers[!streameddata.buf_select][0]));
        fbank.process(buffcopy, WindowStride+overlap, transformoutput);
        offset = strideshape[0];
        memcpy(buffcopy,&streameddata.buffers[!streameddata.buf_select][WindowStride-overlap],
           overlap*sizeof(streameddata.buffers[!streameddata.buf_select][0]));
        dl::TensorBase *transform_tensor = new dl::TensorBase({1, strideshape[0],strideshape[1]}, nullptr, 0, dl::DATA_TYPE_FLOAT);
        transform_tensor->set_element_ptr(transformoutput);
        dl::TensorBase *transposed_tensor = dl::transpose(transform_tensor, {1,3,2});
        model_tensor->push(transposed_tensor, 1);
        if (curr_samples<WindowSamples) continue;
        model_input->assign(model_tensor);
        model->run();
        curr_samples += WindowStride;
        process_model_output(model_output, classification_results[curr_classifications])
        curr_classifications++;
        if (curr_classifications> totnum_classifications)
            ESP_LOGI(TAG, "Predicted the Wrong number of Classifications");
    }
    if(xSemaphoreTake(finishedSaving, portMAX_DELAY) != pdTRUE) ESP_LOGI(TAG, "Took too long to finish Saving");
    // Create the filename for the predictions
    length = snprintf(NULL, 0, "inference_logs_16bit_%s.txt", classification_cases[boot_counter]);
    char *inferencefilename = (char *)malloc((length + 1)* sizeof(char));
    snprintf(inferencefilename, length + 1, "inference_logs_16bit_%s.txt", classification_cases[boot_counter]);
    FILE* inference_logs = init_file(inferencefilename);
    // save the predictions
    for (int i = 0; i<totnum_classifications; i++)
    {
        fprintf(inference_logs,"at %ld ms.:\nPredictions (DSP: %ld ms., Classification: %ld ms.): \n",
                timing_results[i][0], timing_results[i][1], timing_results[i][2]);
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) 
            fprintf(inference_logs, "    %s: %f\n", result.classification[ix].label, classification_results[i][ix]);
    }
    // Save the totals:
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++)
                fprintf(inference_logs, "Total Classification Probabilities above 0.8    %s: %d\n", 
                        result.classification[ix].label, classifiertotals[ix]);
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