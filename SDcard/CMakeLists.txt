if(CONFIG_SD_CARD_MMC)
    set(srcs "SDmmc/sd_card_mmc.c")
elif(CONFIG_SD_CARD_SPI)
    set(srcs "SDspi/sd_card_spi.c")
endif()

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "." "DataFormats"
                    REQUIRES fatfs)
