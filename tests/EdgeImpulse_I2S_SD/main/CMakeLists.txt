idf_component_register(SRCS "main.cpp"
                    REQUIRES EdgeImpulse I2S_SD_ei)

# Define your list of flags
set(my_component_flags
    "-Wno-missing-field-initializers"
    "-Wno-unused-function"
)

target_compile_options(${COMPONENT_LIB} PRIVATE 
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17;${my_component_flags}> 
    $<$<COMPILE_LANGUAGE:C>:${my_component_flags}>
)