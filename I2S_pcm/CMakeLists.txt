idf_component_register(SRCS "I2Sloop/I2Sloop.c" "initI2S/init_i2s.c"
                    REQUIRES esp_driver_i2s esp_driver_gpio
                    INCLUDE_DIRS "." "initI2S" "I2Sloop")
