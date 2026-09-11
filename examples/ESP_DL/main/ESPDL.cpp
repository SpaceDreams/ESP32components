
#include "ESPDL.hpp"
#define GET_transformshape(framesamples) {NUM_MEL_BINS, (framesamples-(FRAME_LENGTH*SAMPLE_RATE))/(FRAME_SHIFT*SAMPLE_RATE)+1} 
#define OVERLAP           (FRAME_LENGTH-FRAME_SHIFT)*(WINDOWSAMPLES/1000)
#define INITSTRIDESHAPE   GET_transformshape(WINDOWSTRIDE)
#define STRIDESHAPE       GET_transformshape(WINDOWSTRIDE+OVERLAP)
 
constexpr uint8_t DIM2LIN(std::initializer_list<int> Sizes,std::initializer_list<int> idx,size_t num) {
	const int* i = idx.begin();
    const int* N = Sizes.begin();
	int dumvar = i[num-1];
	if (dumvar < 0)//If negative then I want to revert the index; like python
		dumvar += N[num-1];
    if(num > 1)
    	dumvar += N[num-1]*DIM2LIN(Sizes,idx,num-1);
    return dumvar;
}


// Transform Configuration
dl::audio::Fbank * transform=nullptr;
float transformoutput[DIM2LIN(STRIDESHAPE,{-1, -1},2)];
//transformoutput is bigger than what's needed for the first iteration, so here I set an offset:
int offset = DIM2LIN(STRIDESHAPE,{-1, -1},2) - DIM2LIN(INITSTRIDESHAPE,{-1, -1},2);
int8_t init_count=0;
//Model Configuration
// The symbol name is composed of three parts: prefix "_binary_", filename "signaldect_espdl", and suffix "_start"
extern const uint8_t model_espdl[] asm("_binary_signaldect_2d_espdl_start"); //
dl::Model *model = nullptr;
// Assigns the first 
dl::TensorBase *model_input = nullptr;
dl::TensorBase *model_output = nullptr;

// not sure the best way to handle the overlap; here I just make an array
float overlapbuff[WINDOWSTRIDE+OVERLAP];
void inittransform(float * input, float * output){
    // Verify input/output buffers are non-null
    if (overlapbuff == nullptr || transform == nullptr) {
        ESP_LOGE("AUDIO", "Audio feature buffers are NULL!");
        return;
    }
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
	const std::vector<int> shape = model_input->get_shape();

    int8_t *tensor_ptr = (int8_t *)model_input->get_element_ptr();
    float scale = DL_RESCALE(model_input->exponent);

    // 1. Shift old quantized tensor data left
    int8_t shiftsize = shape[3]-input_shape[1];
    for (int i = 0; i < shape[3]; i++){
    	//DIM2LIN(shape,{0,0,i,0},4)=shape[3]*i
    	int8_t *start = &tensor_ptr[shape[3]*i];
    	//DIM2LIN(shape,{0,0,i,shiftsize},4)=shape[3]*i+shiftsize
    	int8_t *shiftedblock = &tensor_ptr[shape[3]*i+shiftsize];
        memmove(&start, &shiftedblock, shiftsize * sizeof(tensor_ptr[0]));
    }

    // 2. Quantize new incoming floats directly into the right end of the tensor
    // DIM2LIN(shape,{0,0,0,shiftsize},4) = shiftsize
    int8_t *write_ptr = &tensor_ptr[shiftsize];
    // DIM2LIN(input_shape,{-1,-1},2) = input_shape[1]-1 + input_shape[1]*(input_shape[0]-1)
    int8_t dim2lin = input_shape[1]*input_shape[0]-1;
    for (size_t i = 0; i < dim2lin; i++)
        write_ptr[i] = dl::quantize<int8_t>(normalize(input[i]), scale);
}

// 1. Numerically stable Softmax function
void apply_softmax(const float* input, int size, float* output) {
    // Find the maximum value to prevent float exponential overflow
    float max_val = input[0];
    for (int i = 1; i < size; i++) {
        if (input[i] > max_val) {
            max_val = input[i];
        }
    }

    // Compute the sum of exponents
    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        output[i] = expf(input[i] - max_val);
        sum += output[i];
    }

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

    // Allocate arrays for calculation
    float dequantized_logits[total_elements];

    // Dequantize integers
    for (int i = 0; i < total_elements; i++) 
    	dequantized_logits[i]  = dl::dequantize(raw_output_ptr[i], scale);

    // Apply post-processing Softmax
    apply_softmax(dequantized_logits, total_elements, probabilities);
}

/* These are the public facing functions used to run the model */

void run_classifier_init(){
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
	//config.preemphasis_coefficient=0.0;
    transform = new dl::audio::Fbank(config);

    // Basic usage - loads model with default parameters
    model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    // Assigns the first element from the map
    std::map<std::string, dl::TensorBase *> model_inputs = model->get_inputs();
    model_input = model_inputs.begin()->second;
    std::map<std::string, dl::TensorBase *> model_outputs = model->get_outputs();
    model_output = model_outputs.begin()->second;
}

void run_classifier_continuous(float * input, float *output)
{
	uint16_t strideshape[2] = STRIDESHAPE;
	uint16_t initstrideshape[2] = INITSTRIDESHAPE;
	uint16_t * shape = strideshape;
	if(init_count>0){
		shape = initstrideshape;
		inittransform(input, &transformoutput[offset]);
	} else {
		slicetransform(input,transformoutput);
	}
	shift_and_quantize_direct(transformoutput,shape);
	if (init_count>WINDOWSAMPLES/WINDOWSTRIDE)
	{
		model->run();
		dequantize_model_output(output);
	}
	else
		init_count++;
}