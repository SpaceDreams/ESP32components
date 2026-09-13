
#include "ESPDL.hpp"
#define GET_transformXsize(framesamples) ((framesamples)-(FRAME_LENGTH))/(FRAME_SHIFT)+1
#define OVERLAP           (FRAME_LENGTH-FRAME_SHIFT)*(WINDOWSAMPLES/1000)
#define INITSTRIDESHAPE_X   GET_transformXsize(WINDOWSTRIDE/(WINDOWSAMPLES/1000))
#define STRIDESHAPE_X       GET_transformXsize(WINDOWSTRIDE/(WINDOWSAMPLES/1000)+FRAME_LENGTH-FRAME_SHIFT)
#define INITSTRIDESHAPE_Y   NUM_MEL_BINS
#define STRIDESHAPE_Y       NUM_MEL_BINS
//transformoutput will be bigger than what's needed for the first iteration, so here I set an offset:
#define OFFSET              (STRIDESHAPE_X)*(STRIDESHAPE_Y) - (INITSTRIDESHAPE_X)*(INITSTRIDESHAPE_Y)
static const char *TAG = "ESP-DL";

// Transform Configuration
dl::audio::Fbank * transform=nullptr;
static float transformoutput[(STRIDESHAPE_X)*(STRIDESHAPE_Y)];
static int32_t init_count=0;
// not sure the best way to handle the overlap; here I just make an array
static float overlapbuff[WINDOWSTRIDE+OVERLAP];
//Model Configuration
// The symbol name is composed of three parts: prefix "_binary_", filename "signaldect_espdl", and suffix "_start"
extern const uint8_t model_espdl[] asm("_binary_signaldect_2d_espdl_start"); //
dl::Model *model = nullptr;
// Assigns the first 
dl::TensorBase *model_input = nullptr;
dl::TensorBase *model_output = nullptr;

void inittransform(float * input, float * output){
    memcpy(overlapbuff,&input[WINDOWSTRIDE-OVERLAP],OVERLAP*sizeof(input[0]));
    transform->process(input, WINDOWSTRIDE, output);
}
void slicetransform(float * input, float * output){
	memcpy(&overlapbuff[OVERLAP],input,WINDOWSTRIDE*sizeof(input[0]));
    transform->process(overlapbuff, WINDOWSTRIDE+OVERLAP, output);
    memmove(overlapbuff,&overlapbuff[WINDOWSTRIDE],OVERLAP*sizeof(overlapbuff[0]));
}

float normalize(float x){
	//# 1. Hard clamp extreme values to stabilize the distribution boundary
    //# Typical faucet fbank features sit well between -20 and 80 dB
	float min_val = -20.0f;
    float max_val = 80.0f;
	if (x>max_val)
		x = 80.0f;
	else if(x<min_val)
		x=-20.0f;
	//# 2. Normalize to a predictable, tight range (e.g., -1.0 to 1.0)
    //# This prevents the Power-of-Two scale from leaving huge empty gaps in the INT8 range
    //fbank_normalized = 2.0 * (fbank - (-20.0)) / (80.0 - (-20.0)) - 1.0
    //fbank_normalized = (fbank - (-20.0)) / 50 - 1.0 = fbank/50+2/5-1=fbank/50-3/5
    float multiplier = 0.02f;
    float subtract_val = 0.6f;
    return x*multiplier-subtract_val;
}


void shift_and_quantize_direct(const float *input, uint16_t *input_shape) {
    /** Example Shifting:
     * x={{1,2,3},{4,5,6},{7,8,9}}
     * input = {8,9,10} \\ only considering the case where rows are different but columns are always the same
     * newx = {{4,5,6},{7,8,9},{8,9,10}}
     * inputshape = {1,3}
     * xshape = {3,3}
     * Since the columns of the input are equal to the columns of x and c is column major then I have:
     * newx = memmove(x,&x[flatten(inputshape)],(flatten(xshape)-flatten(inputshape))*size(x[0]))
     * flatten(xshape)=xshape[0]*xshape[1]
     * flatten(inputshape) = inputshape[0]*inputshape[1]
     * **/
	const std::vector<int> shape = model_input->get_shape();
    int8_t *tensor_ptr = (int8_t *)model_input->get_element_ptr();
    float scale = DL_RESCALE(model_input->exponent);
    // 1. Shift old quantized tensor data left
    size_t shiftpoint = input_shape[0]*input_shape[1];
    size_t shiftsize = shape[1]*shape[2] - shiftpoint;
    memmove(tensor_ptr, &tensor_ptr[shiftpoint], shiftsize* sizeof(tensor_ptr[0]));
    // 2. Quantize new incoming floats directly into the right end of the tensor
    int8_t *write_ptr = &tensor_ptr[shiftsize];
    for (size_t i = 0; i < shiftpoint; i++){
            write_ptr[i] = dl::quantize<int8_t>(normalize(input[i]), scale);
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
    // Basic usage - loads model with default parameters
    model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    // Assigns the first element from the map
    std::map<std::string, dl::TensorBase *> model_inputs = model->get_inputs();
    model_input = model_inputs.begin()->second;
    std::map<std::string, dl::TensorBase *> model_outputs = model->get_outputs();
    model_output = model_outputs.begin()->second;
    if(model_output==nullptr || model_input==nullptr || model==nullptr || transform == nullptr){
            ESP_LOGE(TAG, "Error, failed to assign model pointers!");
            return false;
    }
    memset(transformoutput, 0, sizeof(transformoutput));
    memset(overlapbuff, 0, sizeof(overlapbuff));
    return true;
}

void run_classifier_continuous(float * input, float *output)
{
	uint16_t strideshape[2] = {STRIDESHAPE_X,STRIDESHAPE_Y};
	uint16_t initstrideshape[2] = {INITSTRIDESHAPE_X,INITSTRIDESHAPE_Y};
	uint16_t * shape = strideshape;
    float * pnt2transformoutput = transformoutput;
	if(!(init_count>0)){
		shape = initstrideshape;
        pnt2transformoutput = &transformoutput[OFFSET];
		inittransform(input, pnt2transformoutput);
	} else {
		slicetransform(input,pnt2transformoutput);
	}
	shift_and_quantize_direct(pnt2transformoutput,shape);
	if (init_count>=WINDOWSAMPLES)
	{
		model->run();
		dequantize_model_output(output);
    
	}
	else
		init_count += WINDOWSTRIDE;
}