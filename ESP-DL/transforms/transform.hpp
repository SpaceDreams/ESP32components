#include "transform.h"

// Safe Clip and Normalize Macro:
    //# 1. Normalize to a predictable, tight range (e.g., -1.0 to 1.0)
    //# This prevents the Power-of-Two scale from leaving huge empty gaps in the INT8 range
    //fbank_normalized = 2.0 * (fbank - (-20.0)) / (80.0 - (-20.0)) - 1.0
    //fbank_normalized = (fbank - (-20.0)) / 50 - 1.0 = fbank/50+2/5-1=fbank/50-3/5
    //# 2. Hard clamp extreme values to stabilize the distribution boundary
    //# The model set this between -20 and 80 dB
#define NORMALIZE_AND_CLIP(x) ({ \
    float _x = (x);              \
    float _res = _x * 0.02f - 0.6f; \
    /* Hard clamp extreme values */ \
    if (_x > 80.0f)       _res =  1.0f; \
    else if (_x < -20.0f) _res = -1.0f; \
    _res; /* This acts as the return value */ \
})

template <typename T1, typename T2, typename Op=int>
void shift_and_scale(const float *input, uint16_t *input_shape, T1 * output, T2 * output_shape, Op f=0) {
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
    // 1. Shift old quantized tensor data left
    size_t shiftpoint = input_shape[0]*input_shape[1];
    size_t shiftsize = output_shape[0]*output_shape[1] - shiftpoint;
    memmove(output, &output[shiftpoint], shiftsize* sizeof(output[0]));
    T1 *write_ptr = &output[shiftsize];
    // 2. Quantize new incoming floats directly into the right end of the tensor
    for (size_t i = 0; i < shiftpoint; i++){
        float res = NORMALIZE_AND_CLIP(input[i]);
        if constexpr (std::is_invocable_v<Op>)
            write_ptr[i] = static_cast<T1>(f(res));
        else
            write_ptr[i] = static_cast<T1>(res);
    }
}