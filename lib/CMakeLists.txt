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

if(WIN32)
  set(_VIVID_SYSTEM32 "$ENV{SystemRoot}/System32")
  set(VIVID_SYSTEM_DLL_PATHS)
  if(NOT EXISTS "${_VIVID_SYSTEM32}")
    message(WARNING "VIVID: System32 directory not found at '${_VIVID_SYSTEM32}'")
  else()
    set(_VIVID_REQUIRED_SYSTEM_DLLS
      d3dcompiler_47.dll
      vulkan-1.dll
    )
    foreach(_VIVID_DLL IN LISTS _VIVID_REQUIRED_SYSTEM_DLLS)
      # Use unique variable name for each DLL to avoid cache issues
      unset(_VIVID_FOUND_DLL CACHE)
      find_file(_VIVID_FOUND_DLL
        NAMES ${_VIVID_DLL}
        PATHS "${_VIVID_SYSTEM32}"
        NO_DEFAULT_PATH
      )
      if(_VIVID_FOUND_DLL)
        message(STATUS "VIVID: Found system DLL: ${_VIVID_FOUND_DLL}")
        list(APPEND VIVID_SYSTEM_DLL_PATHS "${_VIVID_FOUND_DLL}")
      else()
        message(WARNING "VIVID: Required system DLL '${_VIVID_DLL}' not found in ${_VIVID_SYSTEM32}")
      endif()
      unset(_VIVID_FOUND_DLL CACHE)
    endforeach()
    
    if(VIVID_SYSTEM_DLL_PATHS)
      list(REMOVE_DUPLICATES VIVID_SYSTEM_DLL_PATHS)
      message(STATUS "VIVID: System DLL paths to be copied: ${VIVID_SYSTEM_DLL_PATHS}")
    else()
      message(WARNING "VIVID: No system DLLs found")
    endif()
  endif()
endif()

# option(VIVID_USE_SYSTEM_DAWN "Use a system-installed Dawn package instead of fetching prebuilt assets" ON)
# set(dawn_DIR d:/Dawn/lib/cmake/Dawn)

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
  OPTIONS
    "GLM_BUILD_INSTALL ON"
    "GLM_BUILD_LIBRARY OFF"
  )

# Check if glm was added successfully
if(TARGET glm::glm)
  message(STATUS "✓ glm package added successfully")
  message(STATUS "  glm source dir: ${glm_SOURCE_DIR}")
  message(STATUS "  glm binary dir: ${glm_BINARY_DIR}")
else()
  message(WARNING "✗ glm package failed to add")
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

# flecs
find_package(flecs CONFIG REQUIRED)

# DAWN and SDL3 SETUP
set(VIVID_DAWN_VERSION "20250927.170548")
set(VIVID_DAWN_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/../vendor/dawn")
set(VIVID_DAWN_DOWNLOAD_DIR "${PROJECT_BINARY_DIR}/_deps/dawn")

if(EMSCRIPTEN)
  # download reomoteport file
  set(remoteport_filename "emdawnwebgpu-v${VIVID_DAWN_VERSION}.remoteport.py")
  set(remoteport_url "https://github.com/google/dawn/releases/download/v${VIVID_DAWN_VERSION}/${remoteport_filename}")
  set(remoteport_path "${VIVID_DAWN_OUTPUT_DIR}/${remoteport_filename}")
  _vivid_dawn_download("${remoteport_url}" "${remoteport_path}" "")

  set(VIVID_EMDAWN_REMOTEPORT "${remoteport_path}" CACHE FILEPATH "Path to emdawnwebgpu remote port file")


else()# Native build
  find_package(SDL3 CONFIG REQUIRED)

  # Dawn assets configuration

  if(NOT VIVID_USE_SYSTEM_DAWN)
    find_package(dawn CONFIG REQUIRED) #vcpkg

    # fetch_dawn_assets(
    #   VERSION ${VIVID_DAWN_VERSION}
    #   OUTPUT_DIR ${VIVID_DAWN_OUTPUT_DIR}
    #   DOWNLOAD_DIR ${VIVID_DAWN_DOWNLOAD_DIR}
    # )

    # set(VIVID_DAWN_ROOT_DEFAULT "${VIVID_DAWN_OUTPUT_DIR}/dawn-${VIVID_DAWN_VERSION}")
    # set(VIVID_DAWN_ROOT "${VIVID_DAWN_ROOT_DEFAULT}" CACHE PATH "Path to the Dawn source checkout to integrate")
    # set(VIVID_EMDAWN_REMOTEPORT_DEFAULT "${VIVID_DAWN_OUTPUT_DIR}/emdawnwebgpu-v${VIVID_DAWN_VERSION}.remoteport.py")
    # set(VIVID_EMDAWN_REMOTEPORT "${VIVID_EMDAWN_REMOTEPORT_DEFAULT}" CACHE FILEPATH "Path to emdawnwebgpu remote port file")

    # file(TO_CMAKE_PATH "${VIVID_DAWN_ROOT}" VIVID_DAWN_ROOT)
    # file(TO_CMAKE_PATH "${VIVID_EMDAWN_REMOTEPORT}" VIVID_EMDAWN_REMOTEPORT)

    # if(NOT EXISTS "${VIVID_DAWN_ROOT}")
    #   message(FATAL_ERROR "VIVID_DAWN_ROOT '${VIVID_DAWN_ROOT}' does not exist. Please set it to a valid Dawn checkout.")
    # endif()
    # # DAWN CMake options
    # set(DAWN_USE_GLFW OFF CACHE BOOL "Disable Dawn GLFW helpers when using SDL" FORCE)
    # set(DAWN_FETCH_DEPENDENCIES ON CACHE BOOL "Disable Dawn dependency downloads" FORCE)
    # set(DAWN_BUILD_MONOLITHIC_LIBRARY "STATIC" CACHE STRING "Build Dawn monolithic library as static" FORCE)
    # set(DAWN_BUILD_SAMPLES OFF CACHE BOOL "Disable Dawn samples" FORCE)
    # set(DAWN_BUILD_TESTS OFF CACHE BOOL "Disable Dawn tests" FORCE)
    # set(TINT_BUILD_CMD_TOOLS OFF CACHE BOOL "Disable Tint command line tools" FORCE)
    # set(TINT_BUILD_DOCS OFF CACHE BOOL "Disable Tint documentation" FORCE)
    # set(TINT_BUILD_TESTS OFF CACHE BOOL "Disable Tint tests" FORCE)
    # set(DAWN_ENABLE_INSTALL ON CACHE BOOL "Enable Dawn installation" FORCE)
    # if(NOT APPLE)
    #   set(TINT_BUILD_MSL_WRITER OFF CACHE BOOL "Disable Tint MSL writer" FORCE)
    # endif()
    # if(WIN32)
    #   set(DAWN_FORCE_SYSTEM_COMPONENT_LOAD ON CACHE BOOL "Allow Dawn system component fallback on Windows" FORCE)
    #   set(TINT_BUILD_SPV_READER OFF CACHE BOOL "Disable Tint SPIR-V reader" FORCE)
    #   set(TINT_BUILD_WGSL_READER ON CACHE BOOL "Enable Tint WGSL reader" FORCE)
    #   set(TINT_BUILD_GLSL_WRITER OFF CACHE BOOL "Disable Tint GLSL writer" FORCE)
    #   set(TINT_BUILD_GLSL_VALIDATOR OFF CACHE BOOL "Disable Tint GLSL validator" FORCE)
    #   set(TINT_BUILD_SPV_WRITER ON CACHE BOOL "Enable Tint SPIR-V writer" FORCE)
    #   set(TINT_BUILD_WGSL_WRITER ON CACHE BOOL "Enable Tint WGSL writer" FORCE)
    # endif()
    # # check if WAYLAND is the current Session Type and enable DAWN_USE_WAYLAND Wayland option @compile time
    # # You can override this using: cmake -DDAWN_USE_WAYLAND=X (X = ON | OFF)
    # if(LINUX)
    #   if($ENV{XDG_SESSION_TYPE} MATCHES wayland)
    #       option(DAWN_USE_WAYLAND "Enable support for Wayland surface" ON)
    #   endif()
    # endif()
    # # Integrate Dawn build
    # add_subdirectory("${VIVID_DAWN_ROOT}" ${PROJECT_BINARY_DIR}/dawn EXCLUDE_FROM_ALL)
  # Rely on a Dawn installation that provides DawnConfig.cmake
  else()
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
  

  # VIVID_DAWN_PLATFORM_LIBS
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
endif() # end NOT EMSCRIPTEN


# Create an interface target to expose ImGui include dirs to consumers
# add_library(vivid_imgui INTERFACE)
# target_include_directories(vivid_imgui INTERFACE
#   $<BUILD_INTERFACE:${IMGUI_DIR}>
#   $<BUILD_INTERFACE:${IMGUI_DIR}/backends>
#   $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}/imgui>
#   $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}/imgui/backends>
# )


# ---- Add source files ----

# Note: Explicitly listing source files is best practice for CMake dependency tracking
# Add new files here when created

# Header files
set(headers
  # App headers
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vivid/app/App.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vivid/app/SDL3App.h

  # Render headers
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vivid/render/render_component.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vivid/render/render_systems.h

  # UI headers
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vivid/ui/ui_component.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vivid/ui/ui_system.h

  # Window headers
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vivid/window/window_component.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vivid/window/window_systems.h

  # Input headers
  # ${CMAKE_CURRENT_SOURCE_DIR}/include/vivid/input/input_component.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vivid/input/input_system.h

  # Physics headers
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vivid/physics/physics_component.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vivid/physics/physics_system.h
)

# Source files
set(sources
  # App sources
  ${CMAKE_CURRENT_SOURCE_DIR}/source/app/SDL3App.cpp
  
  # Log sources
  ${CMAKE_CURRENT_SOURCE_DIR}/source/log/log.cpp
  
  # Input sources
  ${CMAKE_CURRENT_SOURCE_DIR}/source/input/input_system.cpp
  
  # Physics sources
  ${CMAKE_CURRENT_SOURCE_DIR}/source/physics/physics_system.cpp
  
  # Render sources
  ${CMAKE_CURRENT_SOURCE_DIR}/source/render/render_systems.cpp
  
  # UI sources
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ui/ui_system.cpp
  
  # Window sources
  ${CMAKE_CURRENT_SOURCE_DIR}/source/window/window_systems.cpp
)




# ---- Create library ----

# Note: for header-only libraries change all PUBLIC flags to INTERFACE and create an interface
# target: add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME} ${headers} ${sources} ${IMGUI_SOURCES} ${IMGUI_HEADERS})
target_compile_definitions(${PROJECT_NAME} PUBLIC IMGUI_IMPL_WEBGPU_BACKEND_DAWN)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

# Include directories for dependencies
target_include_directories(
  ${PROJECT_NAME} 
  PUBLIC 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${IMGUI_DIR}>
    $<BUILD_INTERFACE:${IMGUI_DIR}/backends>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}/imgui>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}/imgui/backends>
  PRIVATE
)

if(NOT APPLE)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
endif()

# native settings
if(NOT EMSCRIPTEN)
  target_link_libraries(${PROJECT_NAME}      
    PUBLIC
      glm::glm
      # vivid_imgui
      # EnTT::EnTT
      dawn::webgpu_dawn
      ${VIVID_DAWN_PLATFORM_LIBS}
      SDL3::SDL3
      flecs::flecs
    PRIVATE
  )

  # ---- Define helper function for copying system DLLs to executable targets ----
  # Users can call vivid_copy_system_dlls_to_target(target_name) to automatically
  # copy required system DLLs to their executable's output directory.
  # This is necessary because Dawn's DynamicLib uses LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
  # which doesn't search System32 by default for security reasons.
  
  if(WIN32 AND VIVID_SYSTEM_DLL_PATHS)
    # Make VIVID_SYSTEM_DLL_PATHS available globally so it can be accessed in subdirectories
    set(VIVID_SYSTEM_DLL_PATHS "${VIVID_SYSTEM_DLL_PATHS}" CACHE INTERNAL "System DLL paths for VIVID")
    
    function(vivid_copy_system_dlls_to_target TARGET_NAME)
      if(NOT TARGET ${TARGET_NAME})
        message(WARNING "VIVID: Target '${TARGET_NAME}' does not exist. Cannot copy system DLLs.")
        return()
      endif()
      
      get_target_property(_TARGET_TYPE ${TARGET_NAME} TYPE)
      if(NOT _TARGET_TYPE STREQUAL "EXECUTABLE")
        message(WARNING "VIVID: Target '${TARGET_NAME}' is not an executable. System DLLs are only needed for executables.")
        return()
      endif()
      
      message(STATUS "VIVID: Setting up DLL copy for target '${TARGET_NAME}'")
      message(STATUS "VIVID: DLL paths: ${VIVID_SYSTEM_DLL_PATHS}")
      
      foreach(_VIVID_DLL_PATH IN LISTS VIVID_SYSTEM_DLL_PATHS)
        get_filename_component(_VIVID_DLL_NAME "${_VIVID_DLL_PATH}" NAME)
        message(STATUS "VIVID: Will copy ${_VIVID_DLL_NAME} to ${TARGET_NAME} output directory")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_VIVID_DLL_PATH}"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/${_VIVID_DLL_NAME}"
          COMMENT "VIVID: Copying ${_VIVID_DLL_NAME} to $<TARGET_FILE_DIR:${TARGET_NAME}>"
        )
      endforeach()
    endfunction()
    
    # Install system DLLs for package distribution
    install(FILES ${VIVID_SYSTEM_DLL_PATHS} DESTINATION bin)
  else()
    # Provide empty function for non-Windows or when DLLs not found
    if(WIN32)
      message(WARNING "VIVID: No system DLLs found. The function vivid_copy_system_dlls_to_target() will do nothing.")
    endif()
    function(vivid_copy_system_dlls_to_target TARGET_NAME)
      # Do nothing on non-Windows platforms or when DLLs not found
    endfunction()
  endif()

  # ---- Create an installable target ----
  # this allows users to install and find the library via `find_package()`.

  # the location where the project's version header will be placed should match the project's regular
  # header paths
  string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)

  # Install vivid_imgui interface target along with VIVID
  # install(TARGETS vivid_imgui EXPORT ${PROJECT_NAME}Targets)


  # Install ImGui headers so consumers can use the exported include paths
  install(
    DIRECTORY ${IMGUI_DIR}/
    DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}/imgui
    COMPONENT "${PROJECT_NAME}_Development"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl"
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
    DEPENDENCIES "flecs 4.1.1" "glm 1.0.1"
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
      glm::glm
      # vivid_imgui
      # EnTT::EnTT
      flecs::flecs
    PRIVATE
    
  )
  add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
endif()
