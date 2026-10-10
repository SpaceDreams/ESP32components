
#include "model.hpp"
static const char *TAG = "ESP-DL Model";
//Model Configuration
dl::Model *model = nullptr;
// Assigns the first 
dl::TensorBase *model_input = nullptr;
dl::TensorBase *model_output = nullptr;

int8_t quantize(float a){
    /**
    * \brief Public facing function used to access the model parameters and quantize the value a
    * 
    *
    * \param a value to be quantized for this model
    */
    return dl::quantize<int8_t>(a, DL_RESCALE(model_input->exponent)); 
}

void quantize_direct(const float *input) {
    /**
    * \brief This function takes an input and quantifies it based off the model and stores it to
    *        the model's input.
    * 
    *
    * \param *input this is the pointer to the input data; it must be the same size as what the
    *               model is expecting; otherwise, this will silently fail.
    */
    const std::vector<int> shape = model_input->get_shape();
    int8_t *tensor_ptr = (int8_t *)model_input->get_element_ptr();
    for (size_t i = 0; i < shape[1]*shape[2]; i++)
            tensor_ptr[i] = quantize(input[i]);
}

void apply_softmax(const float* input, int size, float* output) {
    /**
    * \brief The final step for this model is the softmax function.
    *
    * \details This function applys the softmax safely, first it determines the max value
    *          then instead of explicitly dividing, it subtracts the exponents.
    *          1. Finds the absolute maximum value to prevent float exponential overflow
    *          2. Performs the e division using subtraction of exponents and sums the result
    *          3. divids by the sum so the output is the average
    *
    * \param *input This is the model's output, it contains logits which need to be converted
    * \param size This is the number of classifications, ie: the size of the input
    * \param output This is the array for the final output, it must be the same size as the input
    *               or else it will silently fail.
    */
    // 1. Find the absolute maximum value to prevent float exponential overflow
    float max_val = input[0];
    for (int i = 1; i < size; i++) {
        if (fabsf(input[i]) > fabsf(max_val)) {
            max_val = input[i];
        }
    }

    // 2. Compute the sum of exponents
    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        output[i] = expf(input[i] - max_val);
        sum += output[i];
    }
    // 3. Normalize elements to sum up to 1.0 (ie: 100%)
    for (int i = 0; i < size; i++) {
        output[i] /= sum;
    }
}

// 2. Main processing function for the ESP-DL Output Tensor
void dequantize_model_output(float * probabilities) {
    /**
    * \brief This dequantifies the model output and applies the softmax function to the logits.
    *
    * \details The final step for the model outputs quantified logits, they still need to be
    *           dequantified, and they need to be passed to the softmax function.
    *
    * \param *probabilities This is where the final probabilities will be stored; it should be
    *                       the same size as the model output or else this may silently fail.
    */
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

bool run_classifier_init(const uint8_t * model_espdl){
    /**
    * \brief Basic usage - loads model with default parameters and assumes the model is located in Flash RODATA
    * 
    * \param *model_espdl this is the assembly file that is loaded in the main app script
    */
    model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    ESP_ERROR_CHECK(model->test());
    model->profile_memory();
    // Assigns the input array for the first step in the model
    std::map<std::string, dl::TensorBase *> model_inputs = model->get_inputs(); ///< this assigns all input steps of a model to a map
    model_input = model_inputs.begin()->second;///< this assigns the first input step of a model to model_input
    std::map<std::string, dl::TensorBase *> model_outputs = model->get_outputs(); ///< this assigns all output steps of a model to a map
    model_output = model_outputs.begin()->second;///< this assigns the last output step of a model to model_output
    if(model_output==nullptr || model_input==nullptr || model==nullptr){
            ESP_LOGE(TAG, "Error, failed to assign model pointers!");
            return false;
    }
    return true;
}

void quantize_and_run_classifier(const float * input, float *output)
{
    /**
    * \brief This takes raw data quantifies it, runs the classifier then returns the classification probabilities
    *
    * \param *input this is the input to the model, it should be the same size as the model_input or else it may silently fail.
    * 
    * \param *output this is the array for the final classification probabilities, it should be the same size as the models output or else it may silently fail.
    */
	quantize_direct(input);
	model->run();
	dequantize_model_output(output);
}

void run_classifier(float *output)
{
    /**
    * \brief This should be a public facing function to run the model and output the final results
    * \param *output this is the array for the final classification probabilities, it should be the same size as the models output or else it may silently fail.
    */
    model->run();
    dequantize_model_output(output);
}


ModelInput getModelInput(){
    const std::vector<int> shape = model_input->get_shape();
    int8_t *tensor_ptr = (int8_t *)model_input->get_element_ptr();
    ModelInput res = {.input=tensor_ptr,.shape=&shape[1]};
    return res;
}

