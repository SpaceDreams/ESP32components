/* Edge Impulse Espressif ESP32 Standalone Inference ESP IDF Example
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Include ----------------------------------------------------------------- */
// If your target is limited in memory remove this macro to save 10K RAM
#define EIDSP_QUANTIZE_FILTERBANK   0
#define EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW 2
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "initI2S_ei.h"
#include "initSDwav_ei.h"

#define NUM_CHANNELS        INIT_I2S_SLOT_NUMS
#define BYTE_RATE           ((INIT_AUDIO_SAMPLE_RATE * (INIT_AUDIO_BIT_WIDTH/8)) * NUM_CHANNELS)

/** Audio buffers, pointers and selectors */
typedef struct { // To save space this buffer takes bytes; this way the 3 byte mic is saved directly 
                 //    instead of converting it to a 32bit integer.
    uint8_t *buffers[2]; 
    unsigned char buf_select;
    SemaphoreHandle_t buf_ready;
    // This now will represent contained samples
    unsigned int buf_count;
    // This is the total number of samples the buffer containes (ie: length(buffer[0])/3)
    unsigned int n_samples;
    volatile int swapped_buffers_count; // Increments every time data is written
} inference_t;

inference_t inference;
bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
uint32_t rec_time = 60; // seconds
uint32_t totsamples = rec_time*INIT_AUDIO_SAMPLE_RATE;
uint32_t curr_samples = 0;
static const char* TAG = "EI_TESTS";

extern "C" void audio_inference_callback(uint8_t* raw_buffer, size_t n_bytes)
{  // There is a catch here; i2s is 24 bit; so 
    for (int i = 0; i < n_bytes/3; i++) {
        for (int j = 0; j<3; j++)
            inference.buffers[inference.buf_select][inference.buf_count*3+j] = raw_buffer[i*3+j];
        inference.buf_count++;
        if(inference.buf_count >= inference.n_samples) {
            inference.buf_select ^= 1;
            inference.buf_count = 0;
            //inference.buf_ready = 1;
            xSemaphoreGive(inference.buf_ready);
            inference.swapped_buffers_count++;
        }
    }
}

bool microphone_inference_start(uint32_t n_samples)
{
    inference.buffers[0] = (uint8_t *)malloc(n_samples * 3);
    inference.buffers[1] = (uint8_t *)malloc(n_samples * 3);
    if (inference.buffers[0]==NULL || inference.buffers[1] == NULL) {
        ESP_LOGE(TAG, "Failed to allocate Memory for Inference Buffers!");
        return false;
    }
    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    //inference.buf_ready = 0;
    inference.buf_ready = xSemaphoreCreateBinary();
    if (inference.buf_ready == NULL) {
        ESP_LOGE(TAG, "Failed to create semaphore!");
        return false;
    }
    inference.swapped_buffers_count = 0;

    static struct sampleArgs myArgs = {
        .loop_callback = audio_inference_callback
         };
    xTaskCreatePinnedToCore(
        sample_audio,            // Task function
        "Sample_I2S_data",       // Task name
        20480,                 // Max Bytes required for task // DMA buffer and sample buffer don't count since they were allocated at the program startup
        &myArgs,              // Pointer to your struct of arguments
        1,                    // Task priority
        NULL,                 // Task handle
        1                    // Pin to Core 1 (APP_CPU)
    );

    return true;
}
/**
 * Get raw audio signal data
 */
int microphone_audio_signal_get_data(size_t offset, size_t num_of_samples, float *out_ptr)
{
    // Process the conversion to floats
    for (size_t i = 0; i < num_of_samples; i++) {
        // Unpack the 3 bytes into an unsigned 32-bit integer container (LSB First)
        uint32_t unpacked_data = 0;
        for (int j = 0; j<3; j++)
            unpacked_data |= (uint32_t)inference.buffers[inference.buf_select ^ 1][(offset+i)*3+j]<<(8*j);
        // Perform Sign Extension (Crucial for negative sound wave numbers)
        // A 24-bit signed number has its sign bit at bit position 23.
        // If bit 23 is a 1, the number is negative, and we must fill the top 8 bits with 1s.
        if (unpacked_data & 0x00800000) 
            unpacked_data |= 0xFF000000; // Force top byte to be negative padding
        out_ptr[i] = static_cast<float>(static_cast<int32_t>(unpacked_data));//Cast to a signed int first then float
        int32_t round_trip = static_cast<int32_t>(out_ptr[i]);
        // Step 3: Assert they are perfectly identical
        if (round_trip != unpacked_data) 
            ESP_LOGE(TAG, "Mismatch found at: %ld != %ld", round_trip,unpacked_data);
    }
    //ei_printf("Buffer size: %d, Requested end: %d (offset: %d, num_of_samples: %d)\n", 
      //            inference.n_samples, offset + num_of_samples, offset, num_of_samples);
    return 0;
}

void microphone_inference_end(void)
{
    keep_reading_i2s=false;
    ei_free(inference.buffers[0]);
    ei_free(inference.buffers[1]);
}

FILE* init_wavfile(uint32_t rec_time, const char *filename)
{
    FILE* f = init_file(filename);
    uint32_t flash_samples = INIT_AUDIO_SAMPLE_RATE * rec_time * NUM_CHANNELS;
    const wav_header_t wav_header =
        WAV_HEADER_PCM_DEFAULT(BYTE_RATE * rec_time, INIT_AUDIO_BIT_WIDTH, INIT_AUDIO_SAMPLE_RATE, NUM_CHANNELS);
    // Write the header to the WAV file
    fwrite(&wav_header, sizeof(wav_header), 1, f);
    return f;
}


extern "C" void app_main()
{
    const char *mount_point = mount_sdcard();
    FILE* rec_file = init_wavfile(rec_time, "faucetfile.wav");
    FILE* inference_logs = init_file("inference_logs.txt");
    // summary of inferencing settings (from model_metadata.h)
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: ");
    ei_printf_float((float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf(" ms.\n");
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / (INIT_AUDIO_SAMPLE_RATE/1000));
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));

    run_classifier_init();
    ei_printf("\nStarting continious inference in 2 seconds...\n");

    if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false) {
        ei_printf("ERR: Could not allocate audio buffer (size %d), this could be due to the window length of your model\r\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
        return;
    }

    ei_printf("Recording...\n");

    while (curr_samples<totsamples )
    {
        if (xSemaphoreTake(inference.buf_ready, portMAX_DELAY) != pdTRUE) continue;
        if (inference.swapped_buffers_count > 1)
            ESP_LOGE(TAG, "Data missed! Buffer was swapped %d times before reading!", inference.swapped_buffers_count);
        inference.swapped_buffers_count = 0; 
        signal_t signal;
        signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
        signal.get_data = &microphone_audio_signal_get_data;
        ei_impulse_result_t result = {0};
    
        EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug_nn);
        if (r != EI_IMPULSE_OK) {
            ei_printf("ERR: Failed to run classifier (%d)\n", r);
            return;
        }
        int32_t dumvar = curr_samples+EI_CLASSIFIER_SLICE_SIZE < totsamples ? EI_CLASSIFIER_SLICE_SIZE : totsamples - curr_samples;
        fwrite(inference.buffers[inference.buf_select ^ 1], 1, dumvar*3, rec_file);
        //if (++print_results >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)) {
            // print the predictions
            fprintf(inference_logs,"at %f seconds:\nPredictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
                ((float)curr_samples)/INIT_AUDIO_SAMPLE_RATE,result.timing.dsp, result.timing.classification, result.timing.anomaly);
            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) 
                fprintf(inference_logs,"    %s: %f\n", result.classification[ix].label,result.classification[ix].value);
        #if EI_CLASSIFIER_HAS_ANOMALY == 1
                fprintf(inference_logs, "    anomaly score: %f\n"result.anomaly,);
        #endif
            print_results = 0;
        //}
        curr_samples += EI_CLASSIFIER_SLICE_SIZE;
    }
    // All done, unmount partition and disable SDMMC peripheral
    fclose(rec_file);
    fclose(inference_logs);
    microphone_inference_end();
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card Unmounted");
}

