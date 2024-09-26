SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers -Wall")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/esp32-rotary-encoder)

file(DOWNLOAD https://raw.githubusercontent.com/DavidAntliff/esp32-rotary-encoder/master/rotary_encoder.c
    ${CMAKE_BINARY_DIR}/esp32-rotary-encoder/rotary_encoder.c SHOW_PROGRESS)

file(DOWNLOAD https://raw.githubusercontent.com/DavidAntliff/esp32-rotary-encoder/master/include/rotary_encoder.h
    ${CMAKE_BINARY_DIR}/esp32-rotary-encoder/rotary_encoder.h SHOW_PROGRESS)

idf_component_register(
    SRC_DIRS "src" 
    INCLUDE_DIRS "../include" ${CMAKE_BINARY_DIR}/esp32-rotary-encoder)