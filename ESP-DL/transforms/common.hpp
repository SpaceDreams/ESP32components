
#include "dl_fbank.hpp"
#include "math.h" //need pi use: M_PI
#include "SDcard.h"

// Common Configuration
#define FRAME_LENGTH        20 //[ms] in milliseconds
#define FRAME_SHIFT         10 //[ms]
#define SAMPLE_RATE         48000

// FBANK Configuration:
#define NUM_MEL_BINS        40 //

// Calculations required for Streaming:
#define WINDOWSAMPLES      SAMPLE_RATE // This is set by the model
#define WINDOWSTRIDE        (WINDOWSAMPLES/4) //The minimum number is (frame_length*Sample_Rate)
#define GET_transformXsize(framesamples) (((framesamples)-(FRAME_LENGTH))/(FRAME_SHIFT)+1)
#define OVERLAP           ((FRAME_LENGTH-FRAME_SHIFT)*(WINDOWSAMPLES/1000))//
#define INITSTRIDESHAPE_X   GET_transformXsize((WINDOWSTRIDE/(WINDOWSAMPLES/1000)))
#define STRIDESHAPE_X       GET_transformXsize((WINDOWSTRIDE/(WINDOWSAMPLES/1000))+FRAME_LENGTH-FRAME_SHIFT)
#define INITSTRIDESHAPE_Y   NUM_MEL_BINS
#define STRIDESHAPE_Y       NUM_MEL_BINS

//transformoutput will be bigger than what's needed for the first iteration, so here I set an offset:
#define OFFSET              ((STRIDESHAPE_X)*(STRIDESHAPE_Y) - (INITSTRIDESHAPE_X)*(INITSTRIDESHAPE_Y))

// Transform Configuration
dl::audio::Fbank * transform=nullptr;
static const uint16_t sizeoftransout = (STRIDESHAPE_X)*(STRIDESHAPE_Y);//25*40
static const uint16_t sizeofoverlapbuff = WINDOWSTRIDE+OVERLAP;//12000+480
static const uint16_t sizeofmodelinput = (STRIDESHAPE_X)*(STRIDESHAPE_Y)*3+(INITSTRIDESHAPE_X)*(INITSTRIDESHAPE_Y);

static float transformoutput[sizeoftransout];
// not sure the best way to handle the overlap; here I just make an array
static float overlapbuff[sizeofoverlapbuff];
// I'm choosing a 3 for now because I know that works; but generally that's related to the model_input
static float model_input[sizeofmodelinput];

void inittransform(float * input, float * output){
    memcpy(overlapbuff,&input[WINDOWSTRIDE-OVERLAP],OVERLAP*sizeof(input[0]));
    transform->process(input, WINDOWSTRIDE, output);
}
void slicetransform(float * input, float * output){
	memcpy(&overlapbuff[OVERLAP],input,WINDOWSTRIDE*sizeof(input[0]));
    transform->process(overlapbuff, WINDOWSTRIDE+OVERLAP, output);
    memmove(overlapbuff,&overlapbuff[WINDOWSTRIDE],OVERLAP*sizeof(overlapbuff[0]));
}

float normalize(float x){
	//# 1. Hard clamp extreme values to stabilize the distribution boundary
    //# Typical faucet fbank features sit well between -20 and 80 dB
	float min_val = -20.0f;
    float max_val = 80.0f;
	if (x>max_val)
		x = 80.0f;
	else if(x<min_val)
		x=-20.0f;
	//# 2. Normalize to a predictable, tight range (e.g., -1.0 to 1.0)
    //# This prevents the Power-of-Two scale from leaving huge empty gaps in the INT8 range
    //fbank_normalized = 2.0 * (fbank - (-20.0)) / (80.0 - (-20.0)) - 1.0
    //fbank_normalized = (fbank - (-20.0)) / 50 - 1.0 = fbank/50+2/5-1=fbank/50-3/5
    float multiplier = 0.02f;
    float subtract_val = 0.6f;
    return x*multiplier-subtract_val;
}

template <typename T>
void printValue(T value, size_t val_size ) {
    std::cout << "The value is: " << value << std::endl;
    for (size_t i = 0; i < val_size; i++){
            value[i] = normalize(input[i]);
            //printf("Quantize results: input: %f output: %d\n",input[i],write_ptr[i]);
    }
}

void shift_scale(const float *input, uint16_t *input_shape) {
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
    for (size_t i = 0; i < shiftpoint; i++){
            write_ptr[i] = normalize(input[i]);
            //printf("Quantize results: input: %f output: %d\n",input[i],write_ptr[i]);
        }
}

bool run_classifier_init(){
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
/*
    memset(transformoutput, 0, sizeoftransout*sizeof(transformoutput[0]));
    memset(overlapbuff, 0, sizeofoverlapbuff*sizeof(overlapbuff[0]));
    memset(model_input, 0, sizeofmodelinput*sizeof(model_input[0]));
*/
    return true;
}

int32_t simulated_signal(float * input, int32_t offset)
{
    for (uint32_t i=0;i<WINDOWSTRIDE;i++)
        input[i] = sin(2 * M_PI * 1000 * (offset+i)/SAMPLE_RATE);
    return offset+WINDOWSTRIDE;
}

int32_t simulated_shifting_signal(float * input, int32_t offset)
{
    int32_t count = offset/(WINDOWSTRIDE)-3;
    if (offset<WINDOWSAMPLES) count = 0;
    printf("offset: %ld\n",offset);
    printf("count: %ld\n",count);
    printf("division %ld/%d = %ld\n",offset,WINDOWSTRIDE,offset/(WINDOWSTRIDE));
    int32_t fpicks[] = {100,1000,10000,20000,23000};
    for (uint32_t i=0;i<WINDOWSTRIDE;i++)
        input[i] = sin(2 * M_PI * fpicks[count] * (offset+i)/SAMPLE_RATE);
    return offset+WINDOWSTRIDE;
}

extern "C" void app_main(void){
    run_classifier_init();
    mount_sdcard();
    FILE* f = init_file("simulated_signal_r5.bin");
    uint32_t t_count = 0;
    static float inputbuff[WINDOWSTRIDE]={0};
    ESP_LOGI(TAG, "Made it here");
    t_count = simulated_shifting_signal(inputbuff,t_count);
    inittransform(inputbuff,transformoutput);
    uint16_t initStrideShape[]={INITSTRIDESHAPE_X,INITSTRIDESHAPE_Y};
    shift(transformoutput,initStrideShape);
    uint16_t StrideShape[]={STRIDESHAPE_X,STRIDESHAPE_Y};
    for(int j=0; j<3; j++){
        t_count = simulated_shifting_signal(inputbuff,t_count);
        slicetransform(inputbuff,transformoutput);
        shift(transformoutput,StrideShape);
    }
    fwrite(model_input, sizeof(model_input[0]), sizeof(model_input)/sizeof(model_input[0]) , f);
    for(int j=0; j<4; j++){
        t_count = simulated_shifting_signal(inputbuff,t_count);
        slicetransform(inputbuff,transformoutput);
        shift(transformoutput,StrideShape);
        fwrite(model_input, sizeof(model_input[0]), sizeof(model_input)/sizeof(model_input[0]) , f);
    }
    fclose(f);
    unmount_sdcard();
    
}