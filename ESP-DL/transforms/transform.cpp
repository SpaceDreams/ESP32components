
#include "transform.hpp"

#define OVERLAP           ((FRAME_LENGTH-FRAME_SHIFT)*(WINDOWSAMPLES/1000))//
static const char *TAG = "Transform Tests";
// Transform Configuration
dl::audio::Fbank * transform=nullptr;
static const uint16_t sizeoftransout = (STRIDESHAPE_X)*(STRIDESHAPE_Y);//25*40
static const uint16_t sizeofoverlapbuff = WINDOWSTRIDE+OVERLAP;//12000+480
static const uint16_t sizeofmodelinput = (STRIDESHAPE_X)*(STRIDESHAPE_Y)*3+(INITSTRIDESHAPE_X)*(INITSTRIDESHAPE_Y);

static float transformoutput[sizeoftransout];
// not sure the best way to handle the overlap; here I just make an array
static float overlapbuff[sizeofoverlapbuff];
// Used to set which transform to take
static bool isinit = true;

static TransformResult transout = {.output = transformoutput, .shape = {STRIDESHAPE_X,STRIDESHAPE_Y}};

void inittransform(float * input, float * output){
    memcpy(overlapbuff,&input[WINDOWSTRIDE-OVERLAP],OVERLAP*sizeof(input[0]));
    transform->process(input, WINDOWSTRIDE, output);
}
void slicetransform(float * input, float * output){
	memcpy(&overlapbuff[OVERLAP],input,WINDOWSTRIDE*sizeof(input[0]));
    transform->process(overlapbuff, WINDOWSTRIDE+OVERLAP, output);
    memmove(overlapbuff,&overlapbuff[WINDOWSTRIDE],OVERLAP*sizeof(overlapbuff[0]));
}

inline float normalize(const float x){
    //# 1. Normalize to a predictable, tight range (e.g., -1.0 to 1.0)
    //# This prevents the Power-of-Two scale from leaving huge empty gaps in the INT8 range
    //fbank_normalized = 2.0 * (fbank - (-20.0)) / (80.0 - (-20.0)) - 1.0
    //fbank_normalized = (fbank - (-20.0)) / 50 - 1.0 = fbank/50+2/5-1=fbank/50-3/5
    float multiplier = 0.02f;
    float subtract_val = 0.6f;
    float res = x*multiplier-subtract_val;
    //# 2. Hard clamp extreme values to stabilize the distribution boundary
    //# The model set this between -20 and 80 dB
    float min_val = -20.0f;
    float max_val = 80.0f;
    if (x>max_val)
        res = 1.0f;
    else if(x<min_val)
        res=-1.0f;
    return res;
}

void shift(const float *input, uint16_t *input_shape) {
    /** Example Shifting:
     * x={{1,2,3},{4,5,6},{7,8,9}}
     * input = {8,9,10} \\ only considering the case where rows are different but columns are always the same
     * newx = {{4,5,6},{7,8,9},{8,9,10}}
     * inputshape = {1,3}
     * xshape = {3,3}
     * Since the columns of the input are equal to the columns of x and c is column major then I have:
     * newx = memmove(x,&x[flatten(inputshape)],(flatten(xshape)-flatten(inputshape))*size(x[0]))
     * flatten(xshape)=xshape[0]*xshape[1]
     * flatten(inputshape) = inputshape[0]*inputshape[1]
     * **/
    const std::vector<int> shape = {1,(INITSTRIDESHAPE_X)+3*(STRIDESHAPE_X),NUM_MEL_BINS};
    // 1. Shift old quantized tensor data left
    size_t shiftpoint = input_shape[0]*input_shape[1];
    size_t shiftsize = shape[1]*shape[2] - shiftpoint;
    memmove(model_input, &model_input[shiftpoint], shiftsize* sizeof(model_input[0]));
    // 2. Quantize new incoming floats directly into the right end of the tensor
    float *write_ptr = &model_input[shiftsize];
    for (size_t i = 0; i < shiftpoint; i++)
        write_ptr[i] = normalize(input[i]);
}

bool init_transform(){
    /** Audio buffers, pointers and selectors **/
    // Transform Configuration
    dl::audio::SpeechFeatureConfig config;
    config.sample_rate = SAMPLE_RATE;
    config.frame_length = FRAME_LENGTH;  // ms
    config.frame_shift = FRAME_SHIFT;   // ms
    config.num_mel_bins = NUM_MEL_BINS;
    //config.energy_floor = pow(2,(NOISE_FLOOR/10f)-24)*pow(5,(NOISE_FLOOR/10f));//10**(NOISE_FLOOR/10f)/2**24
    //config.round_to_power_of_two=true;
    config.window_type = dl::audio::WinType::HAMMING;
    //config.dither=0.0;
    config.preemphasis=0.0;
    transform = new dl::audio::Fbank(config);
    if(transform == nullptr){
            ESP_LOGE(TAG, "Error, failed to assign model pointers!");
            return false;
    }
    return true;
}

TransformResult transform(float *input){
    uint16_t strideshape[2] = {STRIDESHAPE_X,STRIDESHAPE_Y};
    uint16_t initstrideshape[2] = {INITSTRIDESHAPE_X,INITSTRIDESHAPE_Y};
    transoutput.shape = strideshape;
    if(isinit){
        transoutput.initstrideshape;
        inittransform(input, transformoutput);
        isinit = !isinit;
    } else
        slicetransform(input,transformoutput);
    return transformoutput;
}

TransformResult transform(float *input, bool init){
    isinit = init;
    return transform(float *input)
}