
#include <dirent.h>
#include "main.hpp"


static const char *TAG = "ESP-DL Testing";
float transformedData[MODELINPUTSHAPE_X*MODELINPUTSHAPE_Y]={0}; //This is big, so initializing now moves off the main segment of dram so it's not included in app memory 

extern "C" void app_main(){
    mount_sdcard();
    const char * faucetoffdir = SD_MOUNT_POINT"/fauceton";
    DIR *nofaucetdir = opendir(faucetoffdir);
    ESP_LOGI(TAG, "Made it here");
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
        run_classifier_continuous(transformedData, classification_results);
        printf("Expected Results is the Facet is on, got: %f faucetoff, %f fauceton\n",classification_results[0],classification_results[1]);
    }
    closedir(nofaucetdir);
    unmount_sdcard();
}