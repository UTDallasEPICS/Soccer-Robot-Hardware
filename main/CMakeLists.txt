SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers -Wall")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

idf_component_register(
    SRC_DIRS "src" 
    INCLUDE_DIRS "../include")