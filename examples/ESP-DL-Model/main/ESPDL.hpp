#include "dl_model_base.hpp"

#define CLASSIFIER_LABEL_COUNT 2
#define CLASSIFIER_LABELS      {"Faucet is Off", "Faucet is On"}
#define MODELINPUTSHAPE_X       (25*3+24) //3*strideslice+initstrideslice
#define MODELINPUTSHAPE_Y        (40)//num_of_mel_banks

void run_classifier_continuous(const float * input, float *output);
bool run_classifier_init();
