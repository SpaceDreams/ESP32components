The Purpose of this component is to provide skeleton code for the C library Edge Impulse creates.
Once the C Library is downloaded include the directorys of the library in this folder.

A usage example is provided in the test directory.

# Steps to Reproduce
1) After creating an Edge Impulse Model download the c++ library
2) There will exist three directories in the c++ library: edge-impulse-sdk,
   model-parameters,tflite-model; copy those directories to this directory.

# Fixing Compilation Errors:
The edge impulse tflite model comes with a compilation error:

```
components/EdgeImpulse/edge-impulse-sdk/classifier/inferencing_engines/tflite_micro.h:154:52: warning: format '%d' expects argument of type 'int', but argument 2 has type 'uint32_t' {aka 'long unsigned int'} [-Wformat=]
  154 |                 "Model provided is schema version %d not equal "
      |                                                   ~^
      |                                                    |
      |                                                    int
      |                                                   %ld
  155 |                 "to supported version %d.",
  156 |                 model->version(), TFLITE_SCHEMA_VERSION);
      |                 ~~~~~~~~~~~~~~~~
      |                               |
      |                               uint32_t {aka long unsigned int}
```