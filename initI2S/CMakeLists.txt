idf_component_register(SRCS "init_I2S.c" "I2Sloop.c"
                    REQUIRES esp_driver_i2s esp_driver_gpio
                    INCLUDE_DIRS ".")
