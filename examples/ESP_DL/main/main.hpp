
#include <math.h>
#include <stdatomic.h> // This is used for the volatile variable
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"

#include "I2Sloop.h"
#include "initSDwav.h"
#define ENABLE_MIC_PIN      GPIO_NUM_14
#define NUM_CHANNELS        INIT_I2S_SLOT_NUMS
#define AUDIO_BIT_WIDTH     INIT_AUDIO_BIT_WIDTH
#define BYTE_RATE           ((INIT_AUDIO_SAMPLE_RATE * (AUDIO_BIT_WIDTH/8)) * NUM_CHANNELS)

// Transform Configuration:
#define FRAME_LENGTH        20 //[ms] in milliseconds
#define FRAME_SHIFT         10 //[ms]
#define SAMPLE_RATE         INIT_AUDIO_SAMPLE_RATE

// For FBANK transfroms:
#define NUM_MEL_BINS        40
#define NOISE_FLOOR         -52.0 // needs to be a float
// Model Setup:
#define WINDOWSAMPLES      SAMPLE_RATE // This is set by the model
#define WINDOWSTRIDE        WINDOWSAMPLES/4 //The minimum number is (frame_length*Sample_Rate)

// Saving data:
#define REC_TIME            10 // seconds
#define TOTSAMPLES          REC_TIME*WINDOWSAMPLES
#define TOT_CLASSIFICATIONS    (TOTSAMPLES - WINDOWSAMPLES)/WINDOWSTRIDE + 1


#include "ESPDL.hpp"

// This file will create two tasks, one which does classifying another which does saving. 
typedef struct { // To save space this buffer takes bytes; this way the 3 byte mic is saved directly 
                 //    instead of converting it to a 32bit integer.
    float buffers[2][WINDOWSTRIDE]; 
    unsigned char buf_select;
    SemaphoreHandle_t buf_ready;
    // This now will represent contained samples
    unsigned int buf_count;
    // This is the total number of samples the buffer containes
    unsigned int n_samples;
    volatile atomic_uint_fast32_t  swapped_buffers_count; // Increments every time data is written
    uint32_t rec_samples;
    bool CompletedSaving;
} record_struct;
record_struct streameddata;
SemaphoreHandle_t finishedSaving;// Used to tell the main app that saving audio data is finished.

void SampleAudioTask(void *ArgPointer){
        sample_audio(ArgPointer);
        vTaskDelete(NULL);
}

FILE* init_wavfile(uint32_t rec_time, const char *filename)
{
    FILE* f = init_file(filename);
    
    // 1. Pre-allocate the entire estimated size (e.g., 10 MB total)
    uint8_t wavheadersize = 44; //bytes
    uint32_t total_file_size = REC_TIME*SAMPLE_RATE*(AUDIO_BIT_WIDTH/8)+wavheadersize;
    fseek(f, total_file_size - 1, SEEK_SET);
    fputc(0, f);
    
    // 2. Rewind to the beginning
    fseek(f, 0, SEEK_SET);
    
    const wav_header_t wav_header =
        WAV_HEADER_PCM_DEFAULT(BYTE_RATE * REC_TIME, AUDIO_BIT_WIDTH, SAMPLE_RATE, NUM_CHANNELS);
    // Write the header to the WAV file
    fwrite(&wav_header, sizeof(wav_header), 1, f);
    return f;
}
