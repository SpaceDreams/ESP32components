#include "dl_model_base.hpp"

#define CLASSIFIER_LABEL_COUNT 2
#define CLASSIFIER_LABELS      {"Faucet is Off", "Faucet is On"}
#define MODELINPUTSHAPE_X       (25*3+24) //3*strideslice+initstrideslice
#define MODELINPUTSHAPE_Y        (40)//num_of_mel_banks

#ifndef MODELFILENAME
#define MODELFILENAME           "signaldect_2D_ESP32"
#endif
// The symbol name is composed of three parts: prefix "_binary_", filename "_espdl", and suffix "_start"
extern const uint8_t model_espdl[] asm("_binary_"MODELFILENAME"_espdl_start"); //
extern void run_classifier_continuous(const float * input, float *output);
extern bool run_classifier_init();
