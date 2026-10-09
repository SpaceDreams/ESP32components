idf_build_get_property(component_targets __COMPONENT_TARGETS)
if ("___idf_espressif__esp-dl" IN_LIST component_targets)
   idf_component_get_property(espdl_dir espressif__esp-dl COMPONENT_DIR)
elseif("___idf_esp-dl" IN_LIST component_targets)
   idf_component_get_property(espdl_dir esp-dl COMPONENT_DIR)
endif()
set(cmake_dir ${espdl_dir}/fbs_loader/cmake)
include(${cmake_dir}/utilities.cmake)
include(${cmake_dir}/data_file_embed_asm_aligned.cmake)

idf_component_register(COMPONENT_NAME "classification"
                    SRCS "model.cpp"
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE -Os -ffunction-sections -fdata-sections)
target_link_options(${COMPONENT_LIB} PRIVATE -Wl,--gc-sections)

set(embed_files signaldect_2D_ESP32.espdl)
target_add_aligned_binary_data(${COMPONENT_LIB} ${embed_files} BINARY)