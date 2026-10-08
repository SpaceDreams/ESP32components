
#include "math.h" //need pi use: M_PI
#include "SDcard.h"
// Transform Configuration:
#define COMMONCONFIG
#define FRAME_LENGTH        20 //[ms] in milliseconds
#define FRAME_SHIFT         10 //[ms]
#define SAMPLE_RATE         48000
// fbank configureation
#define FBANK
#define NUM_MEL_BINS        40 //
#define WINDOWSAMPLES      SAMPLE_RATE // This is set by the model
#define WINDOWSTRIDE        (WINDOWSAMPLES/4) //The minimum number is (frame_length*Sample_Rate)
#include "transform.h"//loaded after the configuration is defined

static const char *TAG = "Transform Tests";

// I'm choosing a 3 for this test; it's related to the window samples
static const uint16_t sizeofmodelinput = (STRIDESHAPE_X)*(STRIDESHAPE_Y)*3+(INITSTRIDESHAPE_X)*(INITSTRIDESHAPE_Y);
static float model_input[sizeofmodelinput];
typedef struct {
    float * input;
    uint16_t shape[2];
} ModelData;
ModelData mod = {.input=model_input,.shape={(STRIDESHAPE_X)*3+(INITSTRIDESHAPE_X),NUM_MEL_BINS}};

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
    init_transform();
    mount_sdcard();
    FILE* f = init_file("simulated_signal.bin");
    uint32_t t_count = 0;
    static float inputbuff[WINDOWSTRIDE]={0};
    t_count = simulated_shifting_signal(inputbuff,t_count);
    TransformResult res = initialtransform(inputbuff);
    shift_and_normalize(res.output,res.shape,mod.input,mod.shape);
    for(int j=0; j<3; j++){
        t_count = simulated_shifting_signal(inputbuff,t_count);
        res = slicetransform(inputbuff);
        shift_and_normalize(res.output,res.shape,mod.input,mod.shape);
    }
    fwrite(mod.input, sizeof(mod.input[0]), mod.shape[0]*mod.shape[1] , f);
    for(int j=0; j<4; j++){
        t_count = simulated_shifting_signal(inputbuff,t_count);
        res = slicetransform(inputbuff);
        shift_and_normalize(res.output,res.shape,mod.input,mod.shape);
        fwrite(mod.input, sizeof(mod.input[0]), mod.shape[0]*mod.shape[1] , f);
    }
    fclose(f);
    unmount_sdcard();
    ESP_LOGE(TAG, "Test Run Complete, you still need to plot and test results in python");
}