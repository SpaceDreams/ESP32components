#include "main.h"

static const char *TAG = "ESP-DL Testing";
// The symbol name is composed of three parts: prefix "_binary_", filename "signaldect_espdl", and suffix "_start"
extern const uint8_t model_espdl[] asm("_binary_"MODELFILENAME"_espdl_start"); //

void SampleAudioTask(void *ArgPointer){
        sample_audio(ArgPointer);
        vTaskDelete(NULL);
}

bool audio_callback(uint8_t* raw_buffer, size_t n_bytes, struct sampleArgs* Args)
{
    /**
    * \brief The function will save data and transfer data to buffers used else where in the app
    *
    * \details This function has to pass two semaphores; one which tells the main function that a buffer is ready.
    *          Another semaphore will tell the main function that saving has completed. 
    *
    * \param raw_buffer Raw data to be transformed or saved
    * \param Args This contains the file name and the other stuff 
    */
    bool keep_reading_i2s = true;
    // The I2S mic is 24 bit; so 
    for (int i = 0; i < n_bytes; i+=3) {
        uint32_t packed_data = 0;
        for (int j = 0; j<3; j++)
            packed_data |= (uint32_t)raw_buffer[i+j]<<(8*j+8);// Added a byte here to easily perform sign extension later
        int32_t res = (int32_t)(packed_data);
        // Perform Sign Extension (Crucial for negative sound wave numbers)
        streameddata.buffers[streameddata.buf_select][streameddata.buf_count] = (float)(res>>8);
        streameddata.buf_count++;
        if(streameddata.buf_count >= streameddata.n_samples) {
            streameddata.buf_select ^= 1;
            streameddata.buf_count = 0;
            // Safely increment by 1 across any CPU core
            atomic_fetch_add(&streameddata.swapped_buffers_count, 1);
            xSemaphoreGive(streameddata.buf_ready);
        }
    }
    // Save here
    if(!atomic_load(&streameddata.CompletedSaving)){
        FILE* f = Args->rec_file;
        size_t dumvar = n_bytes;
        bool completedsaving = false;
        if((streameddata.rec_samples+dumvar/3) > TOTSAMPLES){
            dumvar = (TOTSAMPLES - streameddata.rec_samples)*3;
            atomic_store(&streameddata.CompletedSaving, true);
            completedsaving = true;// Done saving
        }
        fwrite(raw_buffer, sizeof(raw_buffer[0]), dumvar, f);
        streameddata.rec_samples += dumvar/3;
        if(completedsaving){
            fclose(f);
            streameddata.rec_samples = 0;
            ESP_LOGI(TAG, "I2S Data Saved; file closed.");
            xSemaphoreGive(file_closed);
        }
    }
    return keep_reading_i2s;
}

bool microphone_start(uint32_t n_samples, struct sampleArgs * myArgs)
{   
    memset(streameddata.buffers, 0, sizeof(streameddata.buffers));
    streameddata.rec_samples = 0;
    streameddata.buf_select = 0;
    streameddata.buf_count = 0;
    streameddata.n_samples = n_samples;
    atomic_store(&streameddata.CompletedSaving, true);
    file_closed = xSemaphoreCreateBinary();
    streameddata.buf_ready = xSemaphoreCreateBinary();
    if (streameddata.buf_ready == NULL || file_closed == NULL ) {
        ESP_LOGE(TAG, "\nFailed to create semaphore!");
        return false;
    }
    atomic_init(&streameddata.swapped_buffers_count , 0);
    ESP_LOGI(TAG, "Recording is Starting");
    xTaskCreatePinnedToCore(
        SampleAudioTask,            // Task function
        "Sample_I2S_data",       // Task name
        10000,                 // Max Dyanmic Bytes required for task, static bytes are pre-allocated
        myArgs,              // Pointer to your struct of arguments
        1,                    // Task priority
        NULL,                 // Task handle
        1                    // Pin to Core 1 (APP_CPU)
    );
    return true;
}

void app_main(){
    // 1. Reset the pin to its default state
    gpio_reset_pin(ENABLE_MIC_PIN);

    // 2. Set the pin direction to output mode
    gpio_set_direction(ENABLE_MIC_PIN, GPIO_MODE_OUTPUT);

    // 3. Set the pin level to HIGH (1)
    gpio_set_level(ENABLE_MIC_PIN, 1);

    // Here I am opening the file and creating the name:
    mount_sdcard();
    // summary of inferencing settings (from model_metadata.h)
    printf("Transform settings:\n");
    printf("\tInterval: ");
    printf_float((float)WINDOWSAMPLES/(INIT_AUDIO_SAMPLE_RATE/1000));
    printf(" ms.\n");
    printf("\tStride: %d ms.\n", WINDOWSTRIDE / (INIT_AUDIO_SAMPLE_RATE/1000));
    // Now I can initalize the classifier
    if (transform_and_classify_init(model_espdl) == false) {
        ESP_LOGE(TAG, "Issue Initializing Model");
        return;
    }
    static struct sampleArgs theArgs; // For some reason I have to use "static" to pass it to another task
    theArgs.loop_callback = audio_callback;
    // Now I can start the microphone and start recording
    if (microphone_start(WINDOWSTRIDE,&theArgs) == false) {
        ESP_LOGE(TAG, "Issue starting microphone");
        return;
    }
    const char * dummyWAVfile = "dummy.wav";
    const WAVstruct newwav = {.rec_time=REC_TIME, 
                            .sample_rate=INIT_AUDIO_SAMPLE_RATE,
                            .bit_width=INIT_AUDIO_BIT_WIDTH,
                            .num_channels = 1};
    const char *mount_point = SD_MOUNT_POINT;
    const char *boot_key = "boot_counter";
    int32_t iter_val = 0;
    int32_t boot_counter = 0;
    boot_counter = get_nvs_counter(boot_key);
    set_nvs_counter(boot_key, boot_counter+1);
    const char* const class_labels[] = CLASSIFIER_LABELS;
    // Here I am creating variables for saving the classifications:
    float classification_results[TOT_CLASSIFICATIONS][CLASSIFIER_LABEL_COUNT];
    // Header for CSV file:
    // 1. Ask snprintf exactly how many characters it needs
    int required_size[CLASSIFIER_LABEL_COUNT+1];
    int tot_required_size=snprintf(NULL, 0, "time[ms]");
    required_size[0] = tot_required_size+1;// (+1 for the null terminator)
    for (size_t ix = 0; ix < CLASSIFIER_LABEL_COUNT; ix++){
        required_size[ix+1] = snprintf(NULL, 0, ", %s", class_labels[ix])+1;// (+1 for the null terminator)
        tot_required_size+=required_size[ix+1];
    }
    // 2. Allocate the exact size needed
    char* csv_header = (char *)malloc(tot_required_size*sizeof(char));
    char *ptr = csv_header;
    if (!csv_header) return;
    // 3. Write directly into the perfect-sized buffer
    ptr+=snprintf(csv_header,required_size[0],"time[ms]");
    for (size_t ix = 0; ix < CLASSIFIER_LABEL_COUNT; ix++)
        ptr+=snprintf(ptr,required_size[ix+1],", %s", class_labels[ix]);
    while (true){
        int length = snprintf(NULL, 0, "continuous_record_%ld_%ld_espdl.wav",boot_counter, iter_val);
        char *wavfilename = (char *)malloc((length + 1)* sizeof(char));
        snprintf(wavfilename, length + 1, "continuous_record_%ld_%ld_espdl.wav",boot_counter, iter_val);
        FILE* rec_file = init_wavfile(newwav, dummyWAVfile);
        theArgs.rec_file = rec_file;
        atomic_store(&streameddata.CompletedSaving, false);
        uint32_t curr_classifications = 0;
        uint32_t curr_samples =0;
        uint32_t highpoints = 0;
        // now I can loop through everything and it's the same
        while(curr_samples<TOTSAMPLES)
        {
            if (xSemaphoreTake(streameddata.buf_ready, portMAX_DELAY) != pdTRUE) continue;
            uint32_t total_events = atomic_exchange(&streameddata.swapped_buffers_count, 0);
            if (total_events > 1)
                ESP_LOGE(TAG, "Data missed! Buffer was swapped %d times before reading!", total_events);
            transform_and_classify(streameddata.buffers[streameddata.buf_select^1], classification_results[curr_classifications],
                                   curr_samples==0,!(curr_samples<WINDOWSAMPLES));
            curr_samples += WINDOWSTRIDE;
            if (curr_samples<WINDOWSAMPLES) continue;
            curr_classifications++;
            //printf("Classifier Results: Faucet off: %f Faucet on: %f\n",
            //    classification_results[curr_classifications-1][0],
            //    classification_results[curr_classifications-1][1]);
            if(classification_results[curr_classifications-1][1]>0.8)
                highpoints++;
        }
        ESP_LOGI(TAG, "Completed Modeling");
        if(xSemaphoreTake(file_closed, portMAX_DELAY) != pdTRUE) ESP_LOGI(TAG, "Took too long to finish Shutting Down I2S");
        // Rename the dummy file; doing this helps prevent Ghost data from preallocating on the SD card. 
        rename_sdcard_file(dummyWAVfile,wavfilename);

        // Create the filename for the predictions
        length = snprintf(NULL, 0, "inference_logs_%ld_%ld_%ld_espdl.txt",boot_counter, iter_val,highpoints);
        char *inferencefilename = (char *)malloc((length + 1)* sizeof(char));
        snprintf(inferencefilename, length + 1, "inference_logs_%ld_%ld_%ld_espdl.txt",boot_counter, iter_val,highpoints);
        FILE* inference_logs = init_file(inferencefilename);
    
        // save the predictions as a csv file
        fprintf(inference_logs,"%s\n",csv_header);
        for (int i = 0; i<TOT_CLASSIFICATIONS; i++)
        {
            fprintf(inference_logs,"%ld",
                    (uint32_t)(i*WINDOWSTRIDE + WINDOWSAMPLES)/(INIT_AUDIO_SAMPLE_RATE/1000));
            for (size_t ix = 0; ix < CLASSIFIER_LABEL_COUNT; ix++)
                fprintf(inference_logs, ", %f", classification_results[i][ix]);
            fprintf(inference_logs, "\n");
        }
        // Done Saving Predictions
        fclose(inference_logs);
        ESP_LOGI(TAG, "Classification Data Saved");
        // Increment the counter then save
        iter_val++;
    }
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card Unmounted");
}