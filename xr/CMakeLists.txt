cmake_minimum_required(VERSION 3.14...3.22)

project(XrPlugin LANGUAGES CXX)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

# Set CPM cache options before including CPM
set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/../.cpm_cache" CACHE PATH "CPM source cache directory")
set(CPM_USE_LOCAL_PACKAGES ON CACHE BOOL "Use local packages when available")

include(../cmake/CPM.cmake)

# VIVID engine
CPMAddPackage(NAME VIVID SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../lib)

CPMAddPackage(
  NAME OpenXR
  GIT_TAG release-1.1.50
  GITHUB_REPOSITORY KhronosGroup/OpenXR-SDK-Source
  OPTIONS
    "BUILD_API_LAYERS ON"
    "BUILD_TESTS OFF"
  )

# ---- Create standalone executable ----

# Collect all source files including editor
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
file(GLOB_RECURSE common_sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp)

add_executable(${PROJECT_NAME} ${sources} ${common_sources})
add_dependencies(${PROJECT_NAME} XrApiLayer_core_validation)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "XrPlugin")

target_link_libraries(${PROJECT_NAME} 
    VIVID::VIVID 
    openxr_loader
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

# Copy resources to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/res"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/res"
    COMMENT "Copying resources to build directory"
)