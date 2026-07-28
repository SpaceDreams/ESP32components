idf_component_register(SRCS "main.cpp"
                       REQUIRES EdgeImpulse I2S_ei helperfuncs_ei
                      )

# Define your list of flags
set(my_component_flags
    "-Wno-missing-field-initializers"
    "-Wno-unused-function"
)

target_compile_options(${COMPONENT_LIB} PRIVATE 
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17;${my_component_flags}> 
    $<$<COMPILE_LANGUAGE:C>:${my_component_flags}>
)

#Include the below only if you are using the tflite model:
# 1. Fetch the absolute directory of your custom EdgeImpulse component
idf_component_get_property(edge_impulse_dir EdgeImpulse COMPONENT_DIR)

# 2. Tell the main assembler to search inside the EdgeImpulse directory
#    This lets it resolve "tflite-model/tflite_learn_1052597_5.tflite" successfully
target_compile_options(__idf_main PRIVATE "-I${edge_impulse_dir}" "-Wa,-I${edge_impulse_dir}")

