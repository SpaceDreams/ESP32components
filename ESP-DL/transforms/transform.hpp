#include "dl_fbank.hpp"

// Common Configuration:
#define FRAME_LENGTH        20 //[ms] in milliseconds
#define FRAME_SHIFT         10 //[ms]
#define SAMPLE_RATE         48000

// Fbank Configuration:
#define NUM_MEL_BINS        40 //

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
    uint16_t * shape;
} TransformResult;

TransformResult transform(float *input);
TransformResult transform(float *input, bool init);
inline float normalize(const float x);

#ifdef __cplusplus
}
#endif