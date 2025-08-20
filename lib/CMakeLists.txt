cmake_minimum_required(VERSION 3.14...3.22)

# ---- Project ----

project(
  VIVID
  VERSION 1.0
  LANGUAGES CXX
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()


# ---- Add dependencies via CPM ----
# see https://github.com/TheLartians/CPM.cmake for more info

# # Enable CPM source cache to avoid re-downloading packages
# set(CPM_SOURCE_CACHE)

# # Enable using system packages when available
# set(CPM_USE_LOCAL_PACKAGES)

include(../cmake/CPM.cmake)

# # Enable CPM debug output
# set(CPM_DEBUG ON)

# # Set CMake log level to see more details
# if(NOT CMAKE_MESSAGE_LOG_LEVEL)
#   set(CMAKE_MESSAGE_LOG_LEVEL "VERBOSE")
# endif()

# # Print CPM cache directory
# message(STATUS "CPM Source Cache: ${CPM_SOURCE_CACHE}")
# message(STATUS "CPM Use Local Packages: ${CPM_USE_LOCAL_PACKAGES}")

# PackageProject.cmake will be used to make our target installable
message(STATUS "Adding PackageProject.cmake...")
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.8.0")

# Core engine dependencies
message(STATUS "Adding glm...")
CPMAddPackage(
  NAME glm
  GIT_TAG 0.9.9.8
  GITHUB_REPOSITORY g-truc/glm
  )

# Check if glm was added successfully
if(TARGET glm::glm)
  message(STATUS "✓ glm package added successfully")
  message(STATUS "  glm source dir: ${glm_SOURCE_DIR}")
  message(STATUS "  glm binary dir: ${glm_BINARY_DIR}")
else()
  message(WARNING "✗ glm package failed to add")
endif()

message(STATUS "Adding EnTT...")
CPMAddPackage(
  NAME EnTT
  GIT_TAG master
  GITHUB_REPOSITORY skypjack/entt
  OPTIONS
    "ENTT_INSTALL ON"
)

# Check if EnTT was added successfully
if(TARGET EnTT::EnTT)
  message(STATUS "✓ EnTT package added successfully")
  message(STATUS "  EnTT source dir: ${EnTT_SOURCE_DIR}")
  message(STATUS "  EnTT binary dir: ${EnTT_BINARY_DIR}")
else()
  message(WARNING "✗ EnTT package failed to add")
endif()

# message(STATUS "Adding GLFW...")
# CPMAddPackage(
#   NAME glfw
#   GIT_TAG 3.3.8
#   GITHUB_REPOSITORY glfw/glfw
#   OPTIONS
#     "GLFW_BUILD_EXAMPLES OFF"
#     "GLFW_BUILD_TESTS OFF"
#     "GLFW_BUILD_DOCS OFF"
#     "GLFW_INSTALL ON"
# )

# # Check if glfw was added successfully
# if(TARGET glfw)
#   message(STATUS "✓ GLFW package added successfully")
#   message(STATUS "  GLFW source dir: ${glfw_SOURCE_DIR}")
#   message(STATUS "  GLFW binary dir: ${glfw_BINARY_DIR}")
# else()
#   message(WARNING "✗ GLFW package failed to add")
# endif()

# message(STATUS "Adding GLEW...")
# # CPMAddPackage(
# #   NAME glew
# #   GIT_TAG glew-2.2.0
# #   GITHUB_REPOSITORY Perlmint/glew-cmake
# #   OPTIONS
# #     "GLEW_INSTALL ON"
# # )
# CPMAddPackage(NAME glew SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/glew-src/build/cmake)

# Check if glew was added successfully
# if(TARGET glew)
#   message(STATUS "✓ GLEW package added successfully")
#   message(STATUS "  GLEW source dir: ${glew_SOURCE_DIR}")
#   message(STATUS "  GLEW binary dir: ${glew_BINARY_DIR}")
# else()
#   message(WARNING "✗ GLEW package failed to add")
# endif()

# --- Dependencies ---
# Using FetchContent to get GLM and EnTT
# include(FetchContent)
# FetchContent_Declare(
#     glm
#     GIT_REPOSITORY https://github.com/g-truc/glm.git
#     GIT_TAG 0.9.9.8
# )
# FetchContent_Declare(
#     EnTT
#     GIT_REPOSITORY https://github.com/skypjack/entt.git
#     GIT_TAG master
# )
# FetchContent_MakeAvailable(glm EnTT)

# OpenGL and GLEW libraries
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)


# PhysX and Optick (pre-compiled binaries)
set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor)
set(PHYSX_ROOT "${VENDOR_DIR}/PhysX/physx/install/vc17win64/PhysX")
set(PHYSX_INCLUDE_DIR "${PHYSX_ROOT}/include")
set(PHYSX_LIB_DIR "${PHYSX_ROOT}/bin/win.x86_64.vc143.md/release")

set(OPTICK_ROOT "${VENDOR_DIR}/optick")
set(OPTICK_INCLUDE_DIR "${OPTICK_ROOT}/src")
set(OPTICK_LIB_DIR "${OPTICK_ROOT}/lib")

# ---- Add source files ----

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# ---- Create library ----

# Note: for header-only libraries change all PUBLIC flags to INTERFACE and create an interface
# target: add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME} ${headers} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

# Include directories for dependencies
target_include_directories(
  ${PROJECT_NAME} 
  PUBLIC 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${PHYSX_INCLUDE_DIR}
    ${OPTICK_INCLUDE_DIR}
)



if(NOT APPLE)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
endif()

# PhysX libraries
# set(PHYSX_LIBS
#     PhysXFoundation_64
#     PhysXCommon_64
#     PhysX_64
#     PhysXExtensions_static_64
#     PhysXPvdSDK_static_64
#     PhysXGpu_64
#     PhysXCooking_64
#     PhysXCharacterKinematic_static_64
#     PhysXTask_static_64
#     PhysXVehicle2_static_64
#     PVDRuntime_64
# )

# Link directories for PhysX and Optick
link_directories(${PHYSX_LIB_DIR} ${OPTICK_LIB_DIR})

# Create a wrapper interface library for glm to enable export
add_library(vivid_glm INTERFACE)
# Don't link to glm::glm, just set the include directories directly
target_include_directories(vivid_glm INTERFACE 
  $<BUILD_INTERFACE:${glm_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# Update target link libraries to include PhysX, Optick, and GLEW
target_link_libraries(${PROJECT_NAME} 
  PUBLIC
    vivid_glm  # Use our wrapper instead of glm::glm directly
    # glm::glm
    EnTT::EnTT
    GLEW::glew
    glfw

    OpenGL::GL
  PRIVATE
    # Existing libraries
    
    # raylib # Temporarily commented out, conflicts with glfw
    ${PHYSX_LIBS}
    # OptickCore
)

# ---- Create an installable target ----
# this allows users to install and find the library via `find_package()`.

# the location where the project's version header will be placed should match the project's regular
# header paths
string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)

# # Install vivid_glm target along with VIVID
install(TARGETS vivid_glm EXPORT ${PROJECT_NAME}Targets)

# Install glm headers
install(
  DIRECTORY ${glm_SOURCE_DIR}/glm/
  DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}/glm
  COMPONENT "${PROJECT_NAME}_Development"
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.inl"
)

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  VERSION_HEADER "${VERSION_HEADER_LOCATION}"
  COMPATIBILITY SameMajorVersion
  DEPENDENCIES "EnTT master" "glm 0.9.9.8"
)
