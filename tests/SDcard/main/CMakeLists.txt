#set(srcs "sd_card_example_main.c")
set(srcs "initFileTests.c")
idf_component_register(SRCS ${srcs}
                       REQUIRES fatfs SDcard)
