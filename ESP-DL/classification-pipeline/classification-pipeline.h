
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @defgroup ESP-DL
 * 
 * @brief for a given model this will transfrom 1D data and pass it through the model
 * 
 * @details This function will initialize a esp-dl model that was generated with [esp-ppq](https://docs.espressif.com/projects/esp-dl/en/latest/tutorials/how_to_quantize_model.html)
 *         after initialized then the data is transformed into type of 2D fourier transform over a fixed sampling window.
 * @{
 */
void transform_and_classify(float * input, float *output,bool firsttransform, bool continuemodel);
bool transform_and_classify_init(const uint8_t * model_espdl);

/** @} */ // This closes the SDcard group

#ifdef __cplusplus
}
#endif
