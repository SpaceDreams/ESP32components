idf_component_register(SRCS "I2Sloop.c"
                    REQUIRES esp_driver_i2s esp_driver_gpio initI2S
                    INCLUDE_DIRS ".")
