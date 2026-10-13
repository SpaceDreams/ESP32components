
#ifdef __cplusplus
extern "C" {
#endif
void transform_and_classify(float * input, float *output);
bool transform_and_classify_init(const uint8_t * model_espdl);

#ifdef __cplusplus
}
#endif
