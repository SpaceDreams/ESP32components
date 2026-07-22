// If your target is limited in memory remove this macro to save 10K RAM
#define EIDSP_QUANTIZE_FILTERBANK   0
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "i2s2sd_ei.h"

#define EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW 4


/** Audio buffers, pointers and selectors */
typedef struct {
    int32_t *buffers[2]; // The size will be 
    unsigned char buf_select;
    unsigned char buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} inference_t;

inference_t inference;
bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);

void audio_inference_callback(uint32_t n_bytes)
{  // There is a catch here; i2s is 24 bit; so 
    for (size_t i = 0; i < n_bytes/3; i++) {

        // Unpack the 3 bytes into an unsigned 32-bit integer container (MSB First)
        uint32_t unpacked_data = ((uint32_t)i2s_readraw_buff[i]     << 16) |
                                 ((uint32_t)i2s_readraw_buff[i + 1] << 8)  |
                                 ((uint32_t)i2s_readraw_buff[i + 2]);

        // Perform Sign Extension (Crucial for negative sound wave numbers)
        // A 24-bit signed number has its sign bit at bit position 23.
        // If bit 23 is a 1, the number is negative, and we must fill the top 8 bits with 1s.
        if (unpacked_data & 0x800000) {
            unpacked_data |= 0xFF000000; // Force top byte to be negative padding
        }
        inference.buffers[inference.buf_select][inference.buf_count++] = unpacked_data;
        if(inference.buf_count >= inference.n_samples) {
            inference.buf_select ^= 1;
            inference.buf_count = 0;
            inference.buf_ready = 1;
        }
    }
}

bool microphone_inference_start(uint32_t n_samples)
{
    inference.buffers[0] = (int32_t *)malloc(n_samples * sizeof(int32_t));

    if (inference.buffers[0] == NULL) {
        return false;
    }

    inference.buffers[1] = (int32_t *)malloc(n_samples * sizeof(int32_t));

    if (inference.buffers[1] == NULL) {
        ei_free(inference.buffers[0]);
        return false;
    }

    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    struct recordArgs myArgs = {
        .rec_time = 60,
        .filename = "ei_Audio_data.wav",
        .loop_callback = audio_inference_callback
         };
    xTaskCreatePinnedToCore(
        record_wav,            // Task function
        "Record_and_Save_I2S_data",       // Task name
        20000,                 // Max Bytes required for task
        &myArgs,              // Pointer to your struct of arguments
        1,                    // Task priority
        NULL,                 // Task handle
        1                     // Pin to Core 1 (APP_CPU)
    );

    return true;
}

bool microphone_inference_record(void)
{
    bool ret = true;
    if (inference.buf_ready == 1) {
        ei_printf(
            "Error sample buffer overrun. Decrease the number of slices per model window "
            "(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)\n");
        ret = false;
    }
    while (inference.buf_ready == 0) {
        vTaskDelay(1);
    }

    inference.buf_ready = 0;
    return ret;
}

/**
 * Get raw audio signal data
 */
int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
    // Process the conversion to floats
    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = static_cast<float>(inference.buffers[inference.buf_select ^ 1][offset+i]);
    }
    return 0;
}

void microphone_inference_end(void)
{
    //i2s_deinit();
    ei_free(inference.buffers[0]);
    ei_free(inference.buffers[1]);
}