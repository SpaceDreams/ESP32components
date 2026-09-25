
#include <dirent.h> // This is used to search through directories
#include "dl_fbank.hpp"
#include "math.h" //need pi use: M_PI
#include "SDcard.h"
// Transform Configuration:
#define NUM_MEL_BINS        40 //
#define FRAME_LENGTH        20 //[ms] in milliseconds
#define FRAME_SHIFT         10 //[ms]
#define SAMPLE_RATE         48000
#define REC_TIME            2 //[s] in seconds
#define WINDOWSAMPLES      SAMPLE_RATE // This is set by the model
#define WINDOWSTRIDE        (WINDOWSAMPLES/2) //The minimum number is (frame_length*Sample_Rate)
#define GET_transformXsize(framesamples) (((framesamples)-(FRAME_LENGTH))/(FRAME_SHIFT)+1)
#define OVERLAP           ((FRAME_LENGTH-FRAME_SHIFT)*(WINDOWSAMPLES/1000))//
#define INITSTRIDESHAPE_X   GET_transformXsize((WINDOWSTRIDE/(WINDOWSAMPLES/1000)))
#define STRIDESHAPE_X       GET_transformXsize((WINDOWSTRIDE/(WINDOWSAMPLES/1000))+FRAME_LENGTH-FRAME_SHIFT)
#define INITSTRIDESHAPE_Y   NUM_MEL_BINS
#define STRIDESHAPE_Y       NUM_MEL_BINS
//transformoutput will be bigger than what's needed for the first iteration, so here I set an offset:
#define OFFSET              ((STRIDESHAPE_X)*(STRIDESHAPE_Y) - (INITSTRIDESHAPE_X)*(INITSTRIDESHAPE_Y))
#define TOT_CLASSIFICATIONS    (((REC_TIME*SAMPLE_RATE) - WINDOWSAMPLES)/(WINDOWSTRIDE) + 1)
static const char *TAG = "Transform Tests";
// Transform Configuration
dl::audio::Fbank * transform=nullptr;
static const uint16_t sizeoftransout = (STRIDESHAPE_X)*(STRIDESHAPE_Y);//25*40
static const uint16_t sizeofoverlapbuff = WINDOWSTRIDE+OVERLAP;//12000+480

static float transformoutput[sizeoftransout]={0};
// not sure the best way to handle the overlap; here I just make an array
static float overlapbuff[sizeofoverlapbuff]={0};
static float transforminput[WINDOWSTRIDE]={0};

__attribute__((const)) float normalize(const float x){
	//# 1. Hard clamp extreme values to stabilize the distribution boundary
    //# Typical faucet fbank features sit well between -20 and 80 dB
	float min_val = -20.0f;
    float max_val = 80.0f;
    float res = x;
	if (x>max_val)
		res = 80.0f;
	else if(x<min_val)
		res=-20.0f;
	//# 2. Normalize to a predictable, tight range (e.g., -1.0 to 1.0)
    //# This prevents the Power-of-Two scale from leaving huge empty gaps in the INT8 range
    //fbank_normalized = 2.0 * (fbank - (-20.0)) / (80.0 - (-20.0)) - 1.0
    //fbank_normalized = (fbank - (-20.0)) / 50 - 1.0 = fbank/50+2/5-1=fbank/50-3/5
    float multiplier = 0.02f;
    float subtract_val = 0.6f;
    return res*multiplier-subtract_val;
}

void inittransform(float * input, float * output){
    memcpy(overlapbuff,&input[WINDOWSTRIDE-OVERLAP],OVERLAP*sizeof(input[0]));
    transform->process(input, WINDOWSTRIDE, output);
    for (size_t i = 0; i < (INITSTRIDESHAPE_X)*(INITSTRIDESHAPE_Y); i++)
        output[i] = normalize(output[i]);
}
void slicetransform(float * input, float * output){
    memcpy(&overlapbuff[OVERLAP],input,WINDOWSTRIDE*sizeof(input[0]));
    transform->process(overlapbuff, WINDOWSTRIDE+OVERLAP, output);
    memmove(overlapbuff,&overlapbuff[WINDOWSTRIDE],OVERLAP*sizeof(overlapbuff[0]));
    for (size_t i = 0; i < (STRIDESHAPE_X)*(STRIDESHAPE_Y); i++)
        output[i] = normalize(output[i]);
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
/*    ESP_LOGI("DIAG", "transformoutput ptr: %p", (void *)transformoutput);
ESP_LOGI("DIAG", "overlapbuff ptr: %p", (void *)overlapbuff);
ESP_LOGI("DIAG", "model_input ptr: %p", (void *)model_input);
    memset(transformoutput, 0, sizeoftransout*sizeof(transformoutput[0]));
    ESP_LOGI("DIAG", "after setting transformoutput: overlapbuff ptr: %p", (void *)overlapbuff);
    memset(overlapbuff, 0, sizeofoverlapbuff*sizeof(overlapbuff[0]));
    ESP_LOGI("DIAG", "after setting overlapbuff model_input ptr: %p", (void *)model_input);
    memset(model_input, 0, sizeofmodelinput*sizeof(model_input[0]));
    ESP_LOGI(TAG, "Initialized Model");
*/
    return true;
}

void read_and_transform(FILE * fin,FILE * fout){
/* 
    I need a function which does:
    - reads one stride of data from the file f
    - transforms that one stride of data
    - opens binary file
    - write the transformed data to the end of the binary file
*/
    if (fin == NULL || fout==NULL) {
        ESP_LOGE("MAIN", "Failed to open file!");
        return; // Stop here! Do not proceed to read_and_transform
    }
    struct stat st;
    size_t fileSize=0;
    if (fstat(fileno(fin), &st) == 0) {
        fileSize = st.st_size;
    } else {
        ESP_LOGE("FILE", "Failed to get file status");
    }
    const size_t wav_header_size=44;//bytes
    fseek(fin, wav_header_size, SEEK_SET);//The wave header is 44 bytes
    for(int i=0; i<(fileSize-wav_header_size)/3-WINDOWSTRIDE; i+=WINDOWSTRIDE){
        for(int j=0; j<WINDOWSTRIDE; j+=3){
            uint32_t dum =0;
            for(int k=0; k<3; k++)
                dum|=(uint32_t)fgetc(fin)<<k*8;
            // If the 24th bit is 1, it's a negative number; sign-extend it to 32 bits
            if (dum & 0x00800000)
                dum |= 0xFF000000;
            transforminput[j]=(float)dum;
        }
        size_t size_out=(STRIDESHAPE_X)*(STRIDESHAPE_Y);
        if(i==0){
            inittransform(transforminput,transformoutput);
            size_out=(INITSTRIDESHAPE_X)*(INITSTRIDESHAPE_Y);
        }
        else
            slicetransform(transforminput,transformoutput);
        fwrite(transformoutput,sizeof(float),size_out,fout);
    }
}

extern "C" void app_main(void){
    mount_sdcard();
    const char * faucetoffdir = SD_MOUNT_POINT"/TrainingData/fauceton";
    FILE * fout = init_file("FaucetTransforms_On.bin");
    DIR *nofaucetdir = opendir(faucetoffdir);
    if (nofaucetdir == NULL) {
        ESP_LOGE(TAG, "Could Not Open Directory");
        return;
    }
    // Now I can initalize the classifier
    if (run_classifier_init() == false) {
        ESP_LOGE(TAG, "Error Initializing Model");
        return;
    }
    // now I can loop through everything
    struct dirent *entry;
    while((entry = readdir(nofaucetdir)) != NULL)
    {
        if (!(entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN)) continue;
        char *filename = entry->d_name;
        int len = strlen(filename);
        if (!(len > 4 && strcmp(filename + len - 4, ".wav") == 0)) continue;
        int dirlen=strlen(faucetoffdir);
        size_t namesize = (len+1+dirlen+1);
        char * full_path = (char *)malloc(namesize*sizeof(char));
        snprintf(full_path, namesize, "%s/%s", faucetoffdir, filename);
        printf("%s\n",full_path );
        FILE *fin = fopen(full_path, "rb");
        read_and_transform(fin,fout);
        fclose(fin);
    }
    fclose(fout);
    closedir(nofaucetdir);
    unmount_sdcard();
}