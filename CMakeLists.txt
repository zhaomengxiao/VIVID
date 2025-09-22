# this script adds all subprojects to a single build to allow IDEs understand the full project
# structure.

cmake_minimum_required(VERSION 3.14...3.22)

project(BuildAll LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/tools.cmake)

# needed to generate test target
enable_testing()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/standalone ${CMAKE_BINARY_DIR}/standalone)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/xr ${CMAKE_BINARY_DIR}/xr)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/hello_sdl3 ${CMAKE_BINARY_DIR}/hello_sdl3)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test ${CMAKE_BINARY_DIR}/all_test)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/documentation ${CMAKE_BINARY_DIR}/all_documentation)
# Include webgpu directory, to define the 'webgpu' target
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor/webgpu)