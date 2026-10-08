
#include "transform.hpp"

#define OVERLAP           ((FRAME_LENGTH-FRAME_SHIFT)*(WINDOWSAMPLES/1000))
static const char *TAG = "Transform Tests";
// Transform Configuration
dl::audio::Fbank * transform=nullptr;
static const uint16_t sizeoftransout = (STRIDESHAPE_X)*(STRIDESHAPE_Y);//25*40
static const uint16_t sizeofoverlapbuff = WINDOWSTRIDE+OVERLAP;//12000+480
static const uint16_t sizeofmodelinput = (STRIDESHAPE_X)*(STRIDESHAPE_Y)*3+(INITSTRIDESHAPE_X)*(INITSTRIDESHAPE_Y);

static float transformoutput[sizeoftransout];
// not sure the best way to handle the overlap; here I just make an array
static float overlapbuff[sizeofoverlapbuff];

static TransformResult transoutput = {.output = transformoutput, .shape = {STRIDESHAPE_X,STRIDESHAPE_Y}};

void initial_transform(float * input, float * output){
    memcpy(overlapbuff,&input[WINDOWSTRIDE-OVERLAP],OVERLAP*sizeof(input[0]));
    transform->process(input, WINDOWSTRIDE, output);
}
void slice_transform(float * input, float * output){
	memcpy(&overlapbuff[OVERLAP],input,WINDOWSTRIDE*sizeof(input[0]));
    transform->process(overlapbuff, WINDOWSTRIDE+OVERLAP, output);
    memmove(overlapbuff,&overlapbuff[WINDOWSTRIDE],OVERLAP*sizeof(overlapbuff[0]));
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
        ESP_LOGE(TAG, "Error, failed to assign transform pointers!");
        return false;
    }
    return true;
}

TransformResult initialtransform(float *input){
    transoutput.shape[0] = INITSTRIDESHAPE_X;
    transoutput.shape[1] = INITSTRIDESHAPE_Y;
    initial_transform(input, transoutput.output);
    return transoutput;
}

TransformResult slicetransform(float *input){
    transoutput.shape[0] = STRIDESHAPE_X;
    transoutput.shape[1] = STRIDESHAPE_Y;
    slice_transform(input, transoutput.output);
    return transoutput;
}

void shift_and_normalize(const float *input, uint16_t *input_shape, float * output, uint16_t * output_shape){
    shift_and_scale(input,input_shape,output,output_shape);
}