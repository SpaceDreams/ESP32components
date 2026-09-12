#include "dl_model_base.hpp"
#include "dl_fbank.hpp"

#ifndef SAMPLE_RATE
#define SAMPLE_RATE         48000
#endif

#ifndef WINDOWSAMPLES
#define WINDOWSAMPLES       SAMPLE_RATE // This is set by the model
#endif

#ifndef WINDOWSTRIDE
#define WINDOWSTRIDE        WINDOWSAMPLES/4 //The minimum number is (frame_length*Sample_Rate)
#endif


#ifndef transformconfig
#define transformconfig
// Transform Configuration:
#define FRAME_LENGTH        20 //[ms] in milliseconds
#define FRAME_SHIFT         10 //[ms]

// For FBANK transfroms:
#define NUM_MEL_BINS        40
#define NOISE_FLOOR         -52.0 // needs to be a float
#endif

#define CLASSIFIER_LABEL_COUNT 2
#define CLASSIFIER_LABELS      {"Faucet is Off", "Faucet is On"}

void run_classifier_continuous(float * input, float *output);
bool run_classifier_init();
