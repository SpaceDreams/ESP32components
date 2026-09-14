# Register the initialization component
idf_component_register(
    NAME "transforms"
    SRCS "I2Sloop/I2Sloop.c"
)

# Register the looping/data-handling component
idf_component_register(
    NAME "classification"
    SRCS "initI2S/init_i2s.c"
)
