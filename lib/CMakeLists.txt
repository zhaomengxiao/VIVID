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
include(../cmake/DawnAssets.cmake)

option(VIVID_USE_SYSTEM_DAWN "Use a system-installed Dawn package instead of fetching prebuilt assets" ON)
set(dawn_DIR d:/Dawn/lib/cmake/Dawn)
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
  GIT_TAG 1.0.1
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

# ImGui for editor UI - using local vendor copy
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui-docking)
# ImGui headers
set(IMGUI_HEADERS
    ${IMGUI_DIR}/imgui.h
    ${IMGUI_DIR}/imgui_internal.h
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.h
    ${IMGUI_DIR}/backends/imgui_impl_wgpu.h
)
# ImGui source files
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp
)    

# set(SDL3WEBGPU_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/sdl3webgpu)
# set(SDL3WEBGPU_HEADERS
#     ${SDL3WEBGPU_DIR}/sdl3webgpu.h
# )
# set(SDL3WEBGPU_SOURCES
#     ${SDL3WEBGPU_DIR}/sdl3webgpu.c
# )



# libraries


if(NOT EMSCRIPTEN)# Native build
  find_package(SDL3 CONFIG REQUIRED)

  # Dawn assets configuration

if(NOT VIVID_USE_SYSTEM_DAWN)
if(NOT DEFINED VIVID_DAWN_VERSION)
set(VIVID_DAWN_VERSION "20250927.170548")
endif()

if(NOT DEFINED VIVID_DAWN_OUTPUT_DIR)
  set(VIVID_DAWN_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/../vendor/dawn")
endif()

if(NOT DEFINED VIVID_DAWN_DOWNLOAD_DIR)
  set(VIVID_DAWN_DOWNLOAD_DIR "${PROJECT_BINARY_DIR}/_deps/dawn")
endif()

fetch_dawn_assets(
  VERSION ${VIVID_DAWN_VERSION}
  OUTPUT_DIR ${VIVID_DAWN_OUTPUT_DIR}
  DOWNLOAD_DIR ${VIVID_DAWN_DOWNLOAD_DIR}
)

set(VIVID_DAWN_ROOT_DEFAULT "${VIVID_DAWN_OUTPUT_DIR}/dawn-${VIVID_DAWN_VERSION}")
set(VIVID_DAWN_ROOT "${VIVID_DAWN_ROOT_DEFAULT}" CACHE PATH "Path to the Dawn source checkout to integrate")
set(VIVID_EMDAWN_REMOTEPORT_DEFAULT "${VIVID_DAWN_OUTPUT_DIR}/emdawnwebgpu-v${VIVID_DAWN_VERSION}.remoteport.py")
set(VIVID_EMDAWN_REMOTEPORT "${VIVID_EMDAWN_REMOTEPORT_DEFAULT}" CACHE FILEPATH "Path to emdawnwebgpu remote port file")

file(TO_CMAKE_PATH "${VIVID_DAWN_ROOT}" VIVID_DAWN_ROOT)
file(TO_CMAKE_PATH "${VIVID_EMDAWN_REMOTEPORT}" VIVID_EMDAWN_REMOTEPORT)
else()
# Rely on a Dawn installation that provides DawnConfig.cmake
find_package(Threads REQUIRED)
find_package(dawn CONFIG REQUIRED)

if(TARGET dawn::webgpu_dawn)
  get_target_property(_dawn_debug_location dawn::webgpu_dawn IMPORTED_LOCATION_DEBUG)
  if(_dawn_debug_location AND NOT _dawn_debug_location STREQUAL "NOTFOUND")
    foreach(_config RELEASE RELWITHDEBINFO MINSIZEREL)
      get_target_property(_dawn_location dawn::webgpu_dawn IMPORTED_LOCATION_${_config})
      if(NOT _dawn_location OR _dawn_location STREQUAL "NOTFOUND")
        set_target_properties(dawn::webgpu_dawn PROPERTIES MAP_IMPORTED_CONFIG_${_config} Debug)
      endif()
    endforeach()
  endif()
  unset(_dawn_debug_location)
  unset(_dawn_location)
endif()

set(VIVID_DAWN_ROOT "${dawn_PACKAGE_PREFIX_DIR}" CACHE PATH "Path to the Dawn installation" FORCE)
set(VIVID_EMDAWN_REMOTEPORT "${VIVID_DAWN_ROOT}/share/dawn/emdawnwebgpu.remoteport.py" CACHE FILEPATH "Path to emdawnwebgpu remote port file" FORCE)
endif()
  
  if(NOT VIVID_USE_SYSTEM_DAWN)
    if(NOT EXISTS "${VIVID_DAWN_ROOT}")
      message(FATAL_ERROR "VIVID_DAWN_ROOT '${VIVID_DAWN_ROOT}' does not exist. Please set it to a valid Dawn checkout.")
    endif()
  endif()

  if(APPLE)
    set_source_files_properties(${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
    
    set(VIVID_DAWN_PLATFORM_LIBS
      "-framework CoreFoundation"
      "-framework QuartzCore"
      "-framework Metal"
      "-framework MetalKit"
      "-framework Cocoa"
    )
  elseif(WIN32)
    set(VIVID_DAWN_PLATFORM_LIBS d3dcompiler ws2_32 userenv bcrypt ntdll opengl32 Propsys RuntimeObject)
  elseif(UNIX)
    set(VIVID_DAWN_PLATFORM_LIBS m dl)
  else()
    set(VIVID_DAWN_PLATFORM_LIBS)
  endif()

  if(NOT VIVID_USE_SYSTEM_DAWN)
    set(DAWN_USE_GLFW OFF CACHE BOOL "Disable Dawn GLFW helpers when using SDL" FORCE)
    set(DAWN_FETCH_DEPENDENCIES ON CACHE BOOL "Disable Dawn dependency downloads" FORCE)
    set(DAWN_BUILD_MONOLITHIC_LIBRARY "STATIC" CACHE STRING "Build Dawn monolithic library as static" FORCE)
    set(DAWN_BUILD_SAMPLES OFF CACHE BOOL "Disable Dawn samples" FORCE)
    set(DAWN_BUILD_TESTS OFF CACHE BOOL "Disable Dawn tests" FORCE)
    set(TINT_BUILD_CMD_TOOLS OFF CACHE BOOL "Disable Tint command line tools" FORCE)
    set(TINT_BUILD_DOCS OFF CACHE BOOL "Disable Tint documentation" FORCE)
    set(TINT_BUILD_TESTS OFF CACHE BOOL "Disable Tint tests" FORCE)
    set(DAWN_ENABLE_INSTALL ON CACHE BOOL "Enable Dawn installation" FORCE)
    if(NOT APPLE)
      set(TINT_BUILD_MSL_WRITER OFF CACHE BOOL "Disable Tint MSL writer" FORCE)
    endif()
    if(WIN32)
      set(DAWN_FORCE_SYSTEM_COMPONENT_LOAD ON CACHE BOOL "Allow Dawn system component fallback on Windows" FORCE)
      set(TINT_BUILD_SPV_READER OFF CACHE BOOL "Disable Tint SPIR-V reader" FORCE)
      set(TINT_BUILD_WGSL_READER ON CACHE BOOL "Enable Tint WGSL reader" FORCE)
      set(TINT_BUILD_GLSL_WRITER OFF CACHE BOOL "Disable Tint GLSL writer" FORCE)
      set(TINT_BUILD_GLSL_VALIDATOR OFF CACHE BOOL "Disable Tint GLSL validator" FORCE)
      set(TINT_BUILD_SPV_WRITER ON CACHE BOOL "Enable Tint SPIR-V writer" FORCE)
      set(TINT_BUILD_WGSL_WRITER ON CACHE BOOL "Enable Tint WGSL writer" FORCE)
    endif()
  endif()
  # check if WAYLAND is the current Session Type and enable DAWN_USE_WAYLAND Wayland option @compile time
  # You can override this using: cmake -DDAWN_USE_WAYLAND=X (X = ON | OFF)
  if(LINUX)
    if($ENV{XDG_SESSION_TYPE} MATCHES wayland)
        option(DAWN_USE_WAYLAND "Enable support for Wayland surface" ON)
    endif()
  endif()

  if(NOT VIVID_USE_SYSTEM_DAWN)
    add_subdirectory("${VIVID_DAWN_ROOT}" ${PROJECT_BINARY_DIR}/dawn EXCLUDE_FROM_ALL)
  endif()

  # if(NOT TARGET webgpu_dawn OR NOT TARGET webgpu_cpp)
  #   message(FATAL_ERROR "Dawn integration failed: expected targets 'webgpu_dawn' and 'webgpu_cpp' not found.")
  # endif()

  add_library(vivid_webgpu INTERFACE)
  # target_link_libraries(vivid_webgpu INTERFACE webgpu_dawn webgpu_cpp ${VIVID_DAWN_PLATFORM_LIBS})
  if(VIVID_USE_SYSTEM_DAWN)
    target_link_libraries(vivid_webgpu INTERFACE dawn::dawn_public_config)
  else()
    target_include_directories(vivid_webgpu INTERFACE
      $<BUILD_INTERFACE:${VIVID_DAWN_ROOT}/include>
      $<BUILD_INTERFACE:${VIVID_DAWN_ROOT}/include/webgpu>
      $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}/webgpu>
    )
  endif()
endif()

# Create a wrapper interface library for glm to enable export
add_library(vivid_glm INTERFACE)
# Don't link to glm::glm, just set the include directories directly
target_include_directories(
  vivid_glm
  INTERFACE
    $<BUILD_INTERFACE:${glm_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}/glm>
)
# Create an interface target to expose ImGui include dirs to consumers
add_library(vivid_imgui INTERFACE)
target_include_directories(vivid_imgui INTERFACE
  $<BUILD_INTERFACE:${IMGUI_DIR}>
  $<BUILD_INTERFACE:${IMGUI_DIR}/backends>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}/imgui>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}/imgui/backends>
)
target_compile_definitions(vivid_imgui INTERFACE IMGUI_IMPL_WEBGPU_BACKEND_DAWN)

# PhysX and Optick (pre-compiled binaries)
# set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor)
# set(PHYSX_ROOT "${VENDOR_DIR}/PhysX/physx/install/vc17win64/PhysX")
# set(PHYSX_INCLUDE_DIR "${PHYSX_ROOT}/include")
# set(PHYSX_LIB_DIR "${PHYSX_ROOT}/bin/win.x86_64.vc143.md/release")

# set(OPTICK_ROOT "${VENDOR_DIR}/optick")
# set(OPTICK_INCLUDE_DIR "${OPTICK_ROOT}/src")
# set(OPTICK_LIB_DIR "${OPTICK_ROOT}/lib")

# ---- Add source files ----

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# ---- Create library ----

# Note: for header-only libraries change all PUBLIC flags to INTERFACE and create an interface
# target: add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME} ${headers} ${sources} ${IMGUI_SOURCES} ${IMGUI_HEADERS})

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
    # ${PHYSX_INCLUDE_DIR}
    # ${OPTICK_INCLUDE_DIR}
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
# link_directories(${PHYSX_LIB_DIR} ${OPTICK_LIB_DIR})



# # set(WEBGPU_BACKEND "DAWN")

# Include webgpu directory, to define the 'webgpu' target


# Update target link libraries to include PhysX, Optick, and GLEW

# native settings
if(NOT EMSCRIPTEN)
  target_link_libraries(${PROJECT_NAME} 
      
    PUBLIC
      vivid_glm
        # Use our wrapper instead of glm::glm directly
      vivid_imgui
      EnTT::EnTT
      $<$<BOOL:${VIVID_USE_SYSTEM_DAWN}>:dawn::webgpu_dawn>
      $<$<NOT:$<BOOL:${VIVID_USE_SYSTEM_DAWN}>>:webgpu_dawn>
      ${VIVID_DAWN_PLATFORM_LIBS}
      SDL3::SDL3
    PRIVATE
      
      # ${PHYSX_LIBS}
      # OptickCore
  )


  # ---- Create an installable target ----
  # this allows users to install and find the library via `find_package()`.

  # the location where the project's version header will be placed should match the project's regular
  # header paths
  string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)

  # # Install vivid_glm target along with VIVID
  install(TARGETS vivid_glm EXPORT ${PROJECT_NAME}Targets)

  # Install vivid_imgui interface target along with VIVID
  install(TARGETS vivid_imgui EXPORT ${PROJECT_NAME}Targets)

  # Install vivid_webgpu interface target along with VIVID
  install(TARGETS vivid_webgpu EXPORT ${PROJECT_NAME}Targets)

  # # Install and export sdl3webgpu along with VIVID
  # install(
  #   TARGETS sdl3webgpu
  #   EXPORT ${PROJECT_NAME}Targets
  #   ARCHIVE DESTINATION lib
  #   LIBRARY DESTINATION lib
  #   RUNTIME DESTINATION bin
  #   INCLUDES DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}/sdl3webgpu
  # )

  # Install sdl3webgpu public headers
  # install(
  #   FILES ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/sdl3webgpu/sdl3webgpu.h
  #   DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}/sdl3webgpu
  #   COMPONENT "${PROJECT_NAME}_Development"
  # )

  # Install glm headers
  install(
    DIRECTORY ${glm_SOURCE_DIR}/glm/
    DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}/glm
    COMPONENT "${PROJECT_NAME}_Development"
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.inl"
  )

  # Install ImGui headers so consumers can use the exported include paths
  install(
    DIRECTORY ${IMGUI_DIR}/
    DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}/imgui
    COMPONENT "${PROJECT_NAME}_Development"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl"
  )

  # Install sdl3webgpu headers

  # install(
  #   DIRECTORY ${SDL3WEBGPU_DIR}/
  #   DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}/sdl3webgpu
  #   COMPONENT "${PROJECT_NAME}_Development"
  #   FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl"
  # )

  # Install webgpu headers required by vivid_webgpu interface
  install(
    DIRECTORY ${VIVID_DAWN_ROOT}/include/
    DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}/webgpu
    COMPONENT "${PROJECT_NAME}_Development"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
  )

  # install(
  #   FILES ${VIVID_EMDAWN_REMOTEPORT}
  #   DESTINATION share/${PROJECT_NAME}-${PROJECT_VERSION}/emscripten
  #   COMPONENT "${PROJECT_NAME}_Development"
  # )

  # The application's binary must find wgpu.dll or libwgpu.so at runtime,
  # so we automatically copy it (it's called WGPU_RUNTIME_LIB in general)
  # next to the binary.
  # target_copy_webgpu_binaries(${PROJECT_NAME})

  packageProject(
    NAME ${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    NAMESPACE ${PROJECT_NAME}
    BINARY_DIR ${PROJECT_BINARY_DIR}
    INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
    INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
    VERSION_HEADER "${VERSION_HEADER_LOCATION}"
    COMPATIBILITY SameMajorVersion
    DEPENDENCIES "EnTT master" "glm 1.0.1"
  )
# EMSCRIPTEN build
else()
  if(NOT EXISTS "${VIVID_EMDAWN_REMOTEPORT}")
    message(FATAL_ERROR "VIVID_EMDAWN_REMOTEPORT '${VIVID_EMDAWN_REMOTEPORT}' does not exist. Please set it to a valid emdawnwebgpu remote port file.")
  endif()

  # set(CMAKE_EXECUTABLE_SUFFIX ".html") # this is a lib not an executable so we don't need to set the executable suffix
  target_compile_options(${PROJECT_NAME} PUBLIC "-sUSE_SDL=3" "--use-port=${VIVID_EMDAWN_REMOTEPORT}")
  target_compile_definitions(${PROJECT_NAME} PUBLIC IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
  add_compile_options(-sDISABLE_EXCEPTION_CATCHING=1 -DIMGUI_DISABLE_FILE_FUNCTIONS=1)
  target_link_options(${PROJECT_NAME} PRIVATE
    "--use-port=${VIVID_EMDAWN_REMOTEPORT}"
    "-sUSE_SDL=3"  # There is currently no official support internally at emscripten
    "-sWASM=1"
    "-sASYNCIFY=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sNO_EXIT_RUNTIME=0"
    "-sASSERTIONS=1"
    "-sDISABLE_EXCEPTION_CATCHING=1"
    #"-sNO_FILESYSTEM=1"  # sdl3 integrates a filesystem, you need to disable it at SDL3 build time
    "--shell-file=${CMAKE_CURRENT_LIST_DIR}/../emscripten/shell_minimal.html"
  )
  set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "index")

  target_link_libraries(${PROJECT_NAME} 
    PUBLIC
      vivid_glm  # Use our wrapper instead of glm::glm directly
      vivid_imgui
      EnTT::EnTT
    PRIVATE
    # vivid_sdl3webgpu
    
  )
  add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
endif()
