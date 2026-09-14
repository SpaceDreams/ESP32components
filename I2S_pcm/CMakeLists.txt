# Register the initialization component
idf_component_register(
    NAME "I2Sloop"
    SRCS "I2Sloop/I2Sloop.c"
    REQUIRES "initI2S"
)

# Register the looping/data-handling component
idf_component_register(
    NAME "initI2S"
    SRCS "initI2S/init_i2s.c"
)
