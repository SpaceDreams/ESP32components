This example is made only after realizing Edge Impulses upload limitions for WAV files is 16bit. As of this writing no wav files above 16bit are supported on Edge Impluse and after upload it is scaled down non-uniformily so emulating that on a chip is not feasable without more information about Edge Impluses API. 

# Usage Requirements

The default memory for an esp32 main application is approx: 3584 Bytes. At the time of this writing I save the classification results to ram before saving to the sd card; for this reason alone this project needs more ram, about 8kB. It can be changed `idf.py menuconfig` ->Component Config ->ESP System Settings->Main Task Stack Size

This has been tested with a stack size of 8192 Bytes.

# Using the EON Model

no changes are required

# Using the tflite Model

The tflite model needs to be compiled using assembly so two lines are needed in the cmake file inside the "main" directory:
```
idf_component_get_property(edge_impulse_dir EdgeImpulse COMPONENT_DIR)
target_compile_options(__idf_main PRIVATE "-Wa,-I${edge_impulse_dir}")
```

Lastly, the edge impulse tflite model comes with a compilation error:

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

To fix:
1) open `components/EdgeImpulse/edge-impulse-sdk/classifier/inferencing_engines/tflite_micro.h`
2) at line 154 replace `%d` wiith `%ld`