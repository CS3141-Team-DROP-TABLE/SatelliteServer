cmake_minimum_required (VERSION 2.6)
project (NHM_Satellite)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)



include_directories(include src/lib/include)
add_subdirectory(src)

