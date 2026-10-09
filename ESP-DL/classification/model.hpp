#include "dl_model_base.hpp"
#include "model.h"

typedef struct {
    int8_t * input;
    std::vector<int> shape;
} ModelInput;

ModelInput getModelInput();
