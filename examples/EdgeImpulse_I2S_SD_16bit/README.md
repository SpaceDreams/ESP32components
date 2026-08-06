This example is made only after realizing Edge Impulses upload limitions for WAV files is 16bit. As of this writing no wav files above 16bit are supported on Edge Impluse and after upload it is scaled down non-uniformily so emulating that on a chip is not feasable without more information about Edge Impluses API. 

# Using the EON Model

no changes are required

# Using the tflite Model

The tflite model needs to be compiled using assembly so two lines are needed in the cmake file inside the "main" directory:
```
idf_component_get_property(edge_impulse_dir EdgeImpulse COMPONENT_DIR)
target_compile_options(__idf_main PRIVATE "-Wa,-I${edge_impulse_dir}")
```