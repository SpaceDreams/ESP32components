#include <stdatomic.h> // This is used for the volatile variable
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define ENABLE_MIC_PIN      GPIO_NUM_14

// Common Configuration:
#define COMMONCONFIG
#define FRAME_LENGTH        20 //[ms] in milliseconds
#define FRAME_SHIFT         10 //[ms]
#define SAMPLE_RATE         48000
#define WINDOWSAMPLES      SAMPLE_RATE // This is set by the model
#define WINDOWSTRIDE        (WINDOWSAMPLES/4) //The minimum number is (frame_length*Sample_Rate)

// Fbank Configuration:
#define FBANK
#define NUM_MEL_BINS        40 //

// Model Configuration:
#define MODELCONFIG
#define CLASSIFIER_LABEL_COUNT 2
#define CLASSIFIER_LABELS      {"Faucet is Off", "Faucet is On"}
#define MODELFILENAME           CONFIG_MODEL_FILENAME

#include "ESP-DL.h"
#include "I2Sloop.h"
#include "SDcard.h"
#include "NVS_helperfuncs.h"

// Saving data:
#define REC_TIME            60 // seconds
#define TOTSAMPLES          (REC_TIME*SAMPLE_RATE)
#define TOT_CLASSIFICATIONS    (TOTSAMPLES - WINDOWSAMPLES)/(WINDOWSTRIDE) + 1

// This file will creates a task which reads and saves I2S data. 
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
    volatile atomic_bool CompletedSaving;
} record_struct;
record_struct streameddata;
SemaphoreHandle_t file_closed;