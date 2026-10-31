#include "dl_model_base.hpp"
#include "model.h"

typedef struct {
    int8_t * input;
    int* shape;
} ModelInput;

void getModelInput(ModelInput *res);
int8_t quantize(float a);
