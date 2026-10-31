#include "transform.hpp"
#include "model.hpp"
#include "classification-pipeline.h"

bool transform_and_classify_init(const uint8_t * model_espdl){
	bool flag=false;
	if(init_transform() && run_classifier_init(model_espdl))
		flag= true;
	return flag;
}

void transform_and_classify(float * input, float *output,bool firsttransform, bool continuemodel)
{
	 /**
    * \brief This function transforms the input data and if passes the output classifications if continuemodel is true
    *
    * \param input This is the raw i2s data
    * \param output this is the classified data
    * \param firsttransform this is true when the very first transform is performed on data
    * \param continuemodel this is true once the input data to the model is full
    */
	TransformResult *transformout;
	ModelInput *modelinput;
	if(firsttransform){
		transformout = initialtransform(input);
	} else
		transformout= slicetransform(input);
	getModelInput(modelinput);
	shift_and_scale(transformout->output,transformout->shape,
		            modelinput->input,modelinput->shape,quantize);
	if(continuemodel)
    	run_classifier(output);
}
