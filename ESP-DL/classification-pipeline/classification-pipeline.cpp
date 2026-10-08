#include "transform.hpp"
#include "model.hpp"


bool transform_and_classify_init(){
	bool flag=false;
	if(init_transform() && run_classifier_init())
		flag= true;
	return flag;
}

void transform_and_classify(const float * input, float *output)
{
	TransformResult transformout;
	if(!(init_count>0)){
		transformout = transform(input, true);
	} else
		transformout = transform(input);
	ModelInput modelinput = getModelInput();
	shift_and_scale(transformout.output,transformout.shape,
		            modelinput.input,modelinput.shape,quantize);
	if (init_count<WINDOWSAMPLES){
        init_count += WINDOWSTRIDE;
        return;
    }
    run_classifier()
}
