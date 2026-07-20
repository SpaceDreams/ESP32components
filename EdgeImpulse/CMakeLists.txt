cmake_minimum_required(VERSION 3.13.1)

set (CMAKE_CXX_STANDARD 17)
set(MODEL_FOLDER "${CMAKE_CURRENT_LIST_DIR}")
set(EI_SDK_FOLDER "${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk")

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    # enables generic ESP-NN optimizations by Espressif
    add_definitions(-DEI_CLASSIFIER_TFLITE_ENABLE_ESP_NN=1)
    add_definitions(-DEIDSP_USE_ESP_DSP=1)
    # conditionally enable ESP32S3 specific optimizations
    if(${IDF_TARGET} STREQUAL "esp32s3")
        add_definitions(-DEI_CLASSIFIER_TFLITE_ENABLE_ESP_NN_S3=1)
    endif()
endif()

OPTION(DEFINE_DEBUG
    "Build the project using debugging code"
    OFF)
IF(DEFINE_DEBUG)
    MESSAGE("Adding Debug flag...")
    ADD_DEFINITIONS(-DDEBUG)
    SET(CMAKE_BUILD_TYPE Debug)
    MESSAGE("Build type is " ${CMAKE_BUILD_TYPE})
ENDIF(DEFINE_DEBUG)

set(include_dirs
    ${MODEL_FOLDER}
    ${MODEL_FOLDER}/tflite-model
    ${MODEL_FOLDER}/model-parameters
    ${EI_SDK_FOLDER}
    ${EI_SDK_FOLDER}/porting/espressif/esp-dsp/modules/fft/include
    ${EI_SDK_FOLDER}/porting/espressif/esp-dsp/modules/common/include
)

include(${EI_SDK_FOLDER}/cmake/utils.cmake)

RECURSIVE_FIND_FILE_EXCLUDE_DIR(SOURCE_FILES "./edge-impulse-sdk" "CMSIS" "*.cpp")
RECURSIVE_FIND_FILE_EXCLUDE_DIR(MODEL_FILES "./tflite-model" "CMSIS" "*.cpp")
RECURSIVE_FIND_FILE_EXCLUDE_DIR(CC_FILES "./edge-impulse-sdk" "CMSIS" "*.cc")
RECURSIVE_FIND_FILE_EXCLUDE_DIR(C_FILES "./edge-impulse-sdk" "CMSIS" "*.c")

RECURSIVE_FIND_FILE_EXCLUDE_DIR(S_FILES_ESP_NN "${EI_SDK_FOLDER}/porting/espressif/ESP-NN" "CMSIS" "*.s")
RECURSIVE_FIND_FILE_EXCLUDE_DIR(S_FILES_ESP_DSP "${EI_SDK_FOLDER}/porting/espressif/esp-dsp/modules/fft" "CMSIS" "*.s")

list(APPEND SOURCE_FILES ${C_FILES})
list(APPEND SOURCE_FILES ${CC_FILES})
list(APPEND SOURCE_FILES ${MODEL_FILES})
list(APPEND SOURCE_FILES ${S_FILES_ESP_NN})
list(APPEND SOURCE_FILES ${S_FILES_ESP_DSP})

# Define your list of flags
set(my_component_flags
    "-Wno-missing-field-initializers"
    "-Wno-unused-function"
)

list(APPEND my_component_flags "-fdiagnostics-color=always")
list(APPEND my_component_flags "-Wno-unused-variable")
list(APPEND my_component_flags "-Wno-maybe-uninitialized")
list(APPEND my_component_flags "-Wno-error=format")
list(APPEND my_component_flags "-Wno-unused-but-set-variable")
list(APPEND my_component_flags "-Wno-unused-parameter")
list(APPEND my_component_flags "-Wno-error=nonnull-compare")
list(APPEND my_component_flags "-Wno-error=stringop-truncation")
list(APPEND my_component_flags "-Wno-error=uninitialized")
list(APPEND my_component_flags "-Wno-stringop-truncation")
list(APPEND my_component_flags "-Wno-deprecated-declarations")
list(APPEND my_component_flags "-fno-if-conversion")

idf_component_register(SRCS ${SOURCE_FILES}
                       PRIV_REQUIRES esp_timer
                       INCLUDE_DIRS ${include_dirs})

target_compile_options(${COMPONENT_LIB} PRIVATE 
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17;${my_component_flags}> 
    $<$<COMPILE_LANGUAGE:C>:${my_component_flags}>
)