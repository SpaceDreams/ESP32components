
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"


#include "dl_model_base.hpp"
#include "dl_fbank.hpp"

#include "I2Sloop.h"
#include "initSDwav.h"
#define ENABLE_MIC_PIN 14
#define NUM_CHANNELS        INIT_I2S_SLOT_NUMS
#define AUDIO_BIT_WIDTH     INIT_AUDIO_BIT_WIDTH
#define BYTE_RATE           ((INIT_AUDIO_SAMPLE_RATE * (AUDIO_BIT_WIDTH/8)) * NUM_CHANNELS)
#define WindowSamples       INIT_AUDIO_SAMPLE_RATE // This is set by the model
#define WindowStride        WindowSamples/4 //The minimum number is (frame_length*Sample_Rate)

extern const uint8_t model_espdl[] asm("_binary_signaldect_espdl_start"); //

// This file will create two tasks, one which does classifying another which does saving. 
uint32_t rec_time = 60; // seconds
uint32_t totsamples = rec_time*INIT_AUDIO_SAMPLE_RATE;
uint32_t rec_samples = 0;

/** Audio buffers, pointers and selectors */

dl::audio::SpeechFeatureConfig config;
config.sample_rate = INIT_AUDIO_SAMPLE_RATE;
config.frame_length = 20;  // ms
config.frame_shift = 10;   // ms
config.num_mel_bins = 40;
config.raw_energy = self.noise_floor;
config.round_to_power_of_two=true;
config.window_type = dl::audio::WinType::HAMMING;
config.dither=0.0;
config.preemphasis_coefficient=0.0

size_t windowsize = config.frame_shift*config.sample_rate/1000

dl::audio::Fbank transform(config);
// Process audio data
std::vector<int> totshape = transform.get_output_shape(WindowSamples); //audio_length: number of input audio samples 
std::vector<int> firststrideshape = transform.get_output_shape(WindowStride); //audio_length: number of input audio samples
int overlap =  (config.frame_length-config.frame_shift)*(INIT_AUDIO_SAMPLE_RATE/1000);
std::vector<int> strideshape = transform.get_output_shape(WindowStride+overlap); //audio_length: number of input audio samples 
dl::TensorBase model_tensor(
    {1,totshape[0],totshape[1]},
    nullptr,              // Let TensorBase allocate its own memory
    0,                    // output exponent 
    dl::DATA_TYPE_FLOAT,  // Match transforms precision
    true                  // deep = true (TensorBase owns the memory buffer)
);
model_tensor->memset(0); 
typedef struct { // To save space this buffer takes bytes; this way the 3 byte mic is saved directly 
                 //    instead of converting it to a 32bit integer.
    float *buffers[2]; 
    unsigned char buf_select;
    SemaphoreHandle_t buf_ready;
    // This now will represent contained samples
    unsigned int buf_count;
    // This is the total number of samples the buffer containes (ie: length(buffer[0])/3)
    unsigned int n_samples;
    volatile uint8_t swapped_buffers_count; // Increments every time data is written
} streameddata;

SemaphoreHandle_t finishedSaving;// Used to tell the main app that saving audio data is finished.

// 1. Numerically stable Softmax function
void apply_softmax(const float* input, int size, float* output) {
    // Find the maximum value to prevent float exponential overflow
    float max_val = input[0];
    for (int i = 1; i < size; i++) {
        if (input[i] > max_val) {
            max_val = input[i];
        }
    }

    // Compute the sum of exponents
    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        output[i] = expf(input[i] - max_val);
        sum += output[i];
    }

    // Normalize elements to sum up to 1.0 (100%)
    for (int i = 0; i < size; i++) {
        output[i] /= sum;
    }
}

// 2. Main processing function for your ESP-DL Output Tensor
void process_model_output(dl::TensorBase* output_tensor, float * probabilities) {
    // Get the basic details of the output tensor
    int total_elements = output_tensor->get_size();
    
    // Cast the raw array pointer (Use int8_t* if your model is quantized to 8-bits)
    int8_t* raw_output_ptr = (int8_t*)output_tensor->get_element_ptr();

    // Allocate arrays for calculation
    float dequantized_logits[total_elements];

    // Dequantize integers
    for (int i = 0; i < total_elements; i++) {
    dequantized_logits[i] = (float)raw_int8_ptr[i];
    }

    // Apply your post-processing Softmax
    apply_softmax(dequantized_logits, total_elements, probabilities);
}
