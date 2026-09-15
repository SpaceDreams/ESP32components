if(CONFIG_SD_CARD_MMC)
    set(srcs "SDmmc/SDmmc.c")
elseif(CONFIG_SD_CARD_SPI)
    set(srcs "SDspi/SDspi.c")
endif()

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "." "DataFormats"
                    REQUIRES fatfs)
