idf_component_register(SRCS "main.c"
                       REQUIRES espressif__esp-dl classification
		       PRIV_REQUIRES nvs_flash
                      )

