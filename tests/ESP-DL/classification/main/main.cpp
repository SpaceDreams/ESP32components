
#include <dirent.h>
#define MODELCONFIG
#define CLASSIFIER_LABEL_COUNT 2
#define CLASSIFIER_LABELS      {"Faucet is Off", "Faucet is On"}
#define MODELINPUTSHAPE_X       (25*3+24) //3*strideslice+initstrideslice
#define MODELINPUTSHAPE_Y        (40)//num_of_mel_banks
#define MODELFILENAME           "signaldect_2D_ESP32"
#define CLASSIFIERTEST "fauceton"
#include "model.hpp"

static const char *TAG = "ESP-DL Testing";
float transformedData[MODELINPUTSHAPE_X*MODELINPUTSHAPE_Y]={0}; //This is big, so initializing now moves off the main segment of dram so it's not included in app memory 

extern "C" void app_main(){
    mount_sdcard();
    const char * faucetoffdir = SD_MOUNT_POINT"/"CLASSIFIERTEST"_ESP32";
    DIR *nofaucetdir = opendir(faucetoffdir);
    if (nofaucetdir == NULL) {
        ESP_LOGE(TAG, "Could Not Open Directory");
        return;
    }
    // Now I can initalize the classifier
    if (run_classifier_init() == false) {
        ESP_LOGE(TAG, "Issue Initializing Model");
        return;
    }
    // now I can loop through everything
    struct dirent *entry;
    uint16_t modelinputshape[2]={MODELINPUTSHAPE_X,MODELINPUTSHAPE_Y};
    while((entry = readdir(nofaucetdir)) != NULL) 
    {
        if (!(entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN)) continue;
        char *filename = entry->d_name;
        int len = strlen(filename);
        if (!(len > 4 && strcmp(filename + len - 4, ".bin") == 0)) continue;
        int dirlen=strlen(faucetoffdir);
        size_t namesize = (len+1+dirlen+1);
        char * full_path = (char *)malloc(namesize*sizeof(char));
        snprintf(full_path, namesize, "%s/%s", faucetoffdir, filename);
        printf("%s\n",full_path );
        FILE *file = fopen(full_path, "rb");
        fread(transformedData, sizeof(float), modelinputshape[0]*modelinputshape[1], file);
        fclose(file);
        float classification_results[]={0.0f,0.0f};
        quantize_and_run_classifier(transformedData, classification_results);
        printf("Expected Results is "CLASSIFIERTEST", got: %f faucetoff, %f fauceton\n",classification_results[0],classification_results[1]);
    }
    closedir(nofaucetdir);
    unmount_sdcard();
}