#include "dl_fbank.hpp"

// Default Common Configuration:
#ifndef COMMONCONFIG
#define COMMONCONFIG
#define FRAME_LENGTH        20 //[ms] in milliseconds
#define FRAME_SHIFT         10 //[ms]
#define SAMPLE_RATE         48000
#define WINDOWSAMPLES      SAMPLE_RATE // This is set by the model
#define WINDOWSTRIDE        (WINDOWSAMPLES/4) //The minimum number is (frame_length*Sample_Rate)
#endif


// Fbank Configuration:
#ifndef FBANK
#define FBANK
#define NUM_MEL_BINS        40 //
#endif

// Transform Shapes:
#define GET_transformXsize(framesamples) (((framesamples)-(FRAME_LENGTH))/(FRAME_SHIFT)+1)
#define INITSTRIDESHAPE_X   GET_transformXsize((WINDOWSTRIDE/(WINDOWSAMPLES/1000)))
#define STRIDESHAPE_X       GET_transformXsize((WINDOWSTRIDE/(WINDOWSAMPLES/1000))+FRAME_LENGTH-FRAME_SHIFT)
#define INITSTRIDESHAPE_Y   NUM_MEL_BINS
#define STRIDESHAPE_Y       NUM_MEL_BINS

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float * output;
    uint16_t shape[2];
} TransformResult;

bool init_transform();
TransformResult initialtransform(float *input);
TransformResult slicetransform(float *input);
void shift_and_normalize(const float *input, uint16_t *input_shape, float * output, uint16_t * output_shape);

#ifdef __cplusplus
}
#endif