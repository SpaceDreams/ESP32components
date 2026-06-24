idf_component_register(SRCS "init_I2S.c"
                    REQUIRES esp_driver_i2s
		    PRIV_REQUIRES esp_driver_gpio
                    INCLUDE_DIRS ".")
