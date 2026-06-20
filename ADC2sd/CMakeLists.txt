idf_component_register(SRCS "ADC2sd.c"
                    REQUIRES initSDspi initADC
                    INCLUDE_DIRS "." "include")
