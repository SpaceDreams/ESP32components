// If your target is limited in memory remove this macro to save 10K RAM
#define EIDSP_QUANTIZE_FILTERBANK   0
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "initI2S_ei.h"

#define EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW 4


/** Audio buffers, pointers and selectors */
typedef struct { // To save space this buffer takes bytes; this way the 3 byte mic is saved directly 
                 //    instead of converting it to a 32bit integer.
    uint8_t *buffers[2]; 
    unsigned char buf_select;
    unsigned char buf_ready;
    // This now will represent contained samples
    unsigned int buf_count;
    // This is the total number of samples the buffer containes (ie: length(buffer[0])/3)
    unsigned int n_samples;
} inference_t;

inference_t inference;
bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);

void audio_inference_callback(size_t n_bytes)
{  // There is a catch here; i2s is 24 bit; so 
    for (int i = 0; i < n_bytes/3; i++) {
        for (int j = 0; j<3; j++)
            inference.buffers[inference.buf_select][inference.buf_count+j] = i2s_readraw_buff[i+j];
        inference.buf_count++;
        if(inference.buf_count >= inference.n_samples) {
            inference.buf_select ^= 1;
            inference.buf_count = 0;
            inference.buf_ready = 1;
        }
    }
}

bool microphone_inference_start(uint32_t n_samples)
{
    inference.buffers[0] = (uint8_t *)malloc(n_samples * 3);

    inference.buffers[1] = (uint8_t *)malloc(n_samples * 3);

    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    struct sampleArgs myArgs = {
        .loop_callback = audio_inference_callback
         };
    xTaskCreatePinnedToCore(
        sample_audio,            // Task function
        "Sample_I2S_data",       // Task name
        SAMPLE_SIZE*2+10,                 // Max Bytes required for task // The 10 extra bytes is for program to run
        &myArgs,              // Pointer to your struct of arguments
        1,                    // Task priority
        NULL,                 // Task handle
        0                    // Pin to Core 1 (APP_CPU)
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
int microphone_audio_signal_get_data(size_t offset, size_t num_of_samples, float *out_ptr)
{
    // Process the conversion to floats
    for (size_t i = 0; i < num_of_samples; i++) {
        // Unpack the 3 bytes into an unsigned 32-bit integer container (MSB First)
        uint32_t unpacked_data = ((uint32_t)inference.buffers[inference.buf_select ^ 1][(offset+i)*3]     << 16) |
                                 ((uint32_t)inference.buffers[inference.buf_select ^ 1][(offset+i)*3 + 1] << 8)  |
                                 ((uint32_t)inference.buffers[inference.buf_select ^ 1][(offset+i)*3 + 2]);

        // Perform Sign Extension (Crucial for negative sound wave numbers)
        // A 24-bit signed number has its sign bit at bit position 23.
        // If bit 23 is a 1, the number is negative, and we must fill the top 8 bits with 1s.
        if (unpacked_data & 0x800000) 
            unpacked_data |= 0xFF000000; // Force top byte to be negative padding
        out_ptr[i] = static_cast<float>(unpacked_data);
    }
    return 0;
}

void microphone_inference_end(void)
{
    //i2s_deinit();
    ei_free(inference.buffers[0]);
    ei_free(inference.buffers[1]);
}