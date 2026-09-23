
#include "ESPDL.hpp"
static const char *TAG = "ESP-DL";
//Model Configuration
// The symbol name is composed of three parts: prefix "_binary_", filename "signaldect_espdl", and suffix "_start"
extern const uint8_t model_espdl[] asm("_binary_signaldect_2d_efloor_espdl_start"); //
dl::Model *model = nullptr;
// Assigns the first 
dl::TensorBase *model_input = nullptr;
dl::TensorBase *model_output = nullptr;


void quantize_direct(const float *input, uint16_t *input_shape) {
    int8_t *tensor_ptr = (int8_t *)model_input->get_element_ptr();
    for (size_t i = 0; i < input_shape[0]*input_shape[1]; i++){
            tensor_ptr[i] = dl::quantize<int8_t>(input[i], DL_RESCALE(model_input->exponent));
            //printf("Quantize results: input: %f output: %d\n",input[i],write_ptr[i]);
        }
}

// 1. Numerically stable Softmax function
void apply_softmax(const float* input, int size, float* output) {
    // Find the absolute maximum value to prevent float exponential overflow
    float max_val = input[0];
    for (int i = 1; i < size; i++) {
        if (fabsf(input[i]) > fabsf(max_val)) {
            max_val = input[i];
        }
    }

    // Compute the sum of exponents
    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        output[i] = expf(input[i] - max_val);
        sum += output[i];
    }
    printf("\n");

    // Normalize elements to sum up to 1.0 (100%)
    for (int i = 0; i < size; i++) {
        output[i] /= sum;
    }
}

// 2. Main processing function for the ESP-DL Output Tensor
void dequantize_model_output(float * probabilities) {
    // Get the basic details of the output tensor
    int total_elements = model_output->get_size();
    float scale = DL_SCALE(model_output->exponent);
    // Cast the raw array pointer (Use int8_t* since the model is quantized to 8-bits)
    int8_t* raw_output_ptr = (int8_t*)model_output->get_element_ptr();
    printf("Logits Results: Faucet off: %d Faucet on: %d\n",raw_output_ptr[0],raw_output_ptr[1]);
    // Allocate arrays for calculation
    float dequantized_logits[total_elements];

    // Dequantize integers
    for (int i = 0; i < total_elements; i++)
        dequantized_logits[i]  = dl::dequantize(raw_output_ptr[i], scale);

    // Apply post-processing Softmax
    apply_softmax(dequantized_logits, total_elements, probabilities);
}

/* These are the public facing functions used to run the model */

bool run_classifier_init(){
    // Basic usage - loads model with default parameters
    model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    ESP_ERROR_CHECK(model->test());
    model->profile_memory();
    // Assigns the first element from the map
    std::map<std::string, dl::TensorBase *> model_inputs = model->get_inputs();
    model_input = model_inputs.begin()->second;
    std::map<std::string, dl::TensorBase *> model_outputs = model->get_outputs();
    model_output = model_outputs.begin()->second;
    if(model_output==nullptr || model_input==nullptr || model==nullptr){
            ESP_LOGE(TAG, "Error, failed to assign model pointers!");
            return false;
    }
    return true;
}

void run_classifier_continuous(const float * input, float *output)
{
	uint16_t modelshape[2] = {MODELINPUTSHAPE_X,MODELINPUTSHAPE_Y};
	quantize_direct(input,modelshape);
	model->run();
	dequantize_model_output(output);
}