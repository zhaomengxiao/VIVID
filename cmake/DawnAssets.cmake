include_guard(GLOBAL)

include(CMakeParseArguments)

function(_vivid_dawn_download url destination hash)
  if(hash)
    set(hash_args EXPECTED_HASH SHA256=${hash})
  else()
    set(hash_args)
  endif()

  file(DOWNLOAD
    "${url}"
    "${destination}"
    SHOW_PROGRESS
    INACTIVITY_TIMEOUT 30
    TIMEOUT 600
    STATUS download_status
    ${hash_args}
  )

  list(GET download_status 0 status_code)
  if(NOT "${status_code}" STREQUAL "0")
    list(GET download_status 1 status_message)
    file(REMOVE "${destination}")
    message(FATAL_ERROR "Failed to download '${url}': ${status_message}")
  endif()
endfunction()

function(fetch_dawn_assets)
  set(options)
  set(oneValueArgs VERSION OUTPUT_DIR REMOTEPORT_HASH SOURCE_HASH DOWNLOAD_DIR)
  cmake_parse_arguments(FETCH "${options}" "${oneValueArgs}" "" ${ARGN})

  if(NOT FETCH_VERSION)
    message(FATAL_ERROR "fetch_dawn_assets requires VERSION argument.")
  endif()

  if(NOT FETCH_OUTPUT_DIR)
    message(FATAL_ERROR "fetch_dawn_assets requires OUTPUT_DIR argument.")
  endif()

  if(IS_ABSOLUTE "${FETCH_OUTPUT_DIR}")
    set(output_dir "${FETCH_OUTPUT_DIR}")
  else()
    get_filename_component(output_dir "${FETCH_OUTPUT_DIR}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
  endif()

  if(FETCH_DOWNLOAD_DIR)
    if(IS_ABSOLUTE "${FETCH_DOWNLOAD_DIR}")
      set(download_root "${FETCH_DOWNLOAD_DIR}")
    else()
      get_filename_component(download_root "${FETCH_DOWNLOAD_DIR}" ABSOLUTE BASE_DIR "${CMAKE_BINARY_DIR}")
    endif()
  else()
    set(download_root "${CMAKE_BINARY_DIR}/_dawn_cache/${FETCH_VERSION}")
  endif()
  file(MAKE_DIRECTORY "${download_root}")

  set(remoteport_filename "emdawnwebgpu-v${FETCH_VERSION}.remoteport.py")
  set(remoteport_url "https://github.com/google/dawn/releases/download/v${FETCH_VERSION}/${remoteport_filename}")
  set(remoteport_path "${download_root}/${remoteport_filename}")

  if(EXISTS "${remoteport_path}")
    message(STATUS "Reusing cached '${remoteport_filename}'")
  else()
    message(STATUS "Downloading '${remoteport_filename}' from Dawn releases")
    _vivid_dawn_download("${remoteport_url}" "${remoteport_path}" "${FETCH_REMOTEPORT_HASH}")
  endif()

  set(source_archive_name "dawn-v${FETCH_VERSION}.zip")
  set(source_archive_url "https://github.com/google/dawn/archive/refs/tags/v${FETCH_VERSION}.zip")
  set(source_archive_path "${download_root}/${source_archive_name}")

  if(EXISTS "${source_archive_path}")
    message(STATUS "Reusing cached Dawn source archive '${source_archive_name}'")
  else()
    message(STATUS "Downloading Dawn source archive '${source_archive_name}'")
    _vivid_dawn_download("${source_archive_url}" "${source_archive_path}" "${FETCH_SOURCE_HASH}")
  endif()

  file(SHA256 "${remoteport_path}" remoteport_sha)
  file(SHA256 "${source_archive_path}" source_archive_sha)

  set(expected_stamp "VERSION=${FETCH_VERSION}\nREMOTEPORT_SHA=${remoteport_sha}\nSOURCE_SHA=${source_archive_sha}\n")
  set(stamp_file "${output_dir}/.dawn_assets.stamp")

  set(extraction_needed ON)
  if(EXISTS "${stamp_file}")
    file(READ "${stamp_file}" stamp_contents)
    if(stamp_contents STREQUAL expected_stamp)
      if(EXISTS "${output_dir}" AND IS_DIRECTORY "${output_dir}")
        set(extraction_needed OFF)
        message(STATUS "Dawn assets already up to date at '${output_dir}'. Skipping extraction.")
      endif()
    endif()
  endif()

  if(extraction_needed)
    message(STATUS "Preparing Dawn assets directory '${output_dir}'")
    if(EXISTS "${output_dir}")
      file(GLOB output_entries "${output_dir}/*")

      set(preserve_paths)
      if(EXISTS "${source_archive_path}")
        file(TO_CMAKE_PATH "${source_archive_path}" source_archive_norm)
        list(APPEND preserve_paths "${source_archive_norm}")
      endif()
      if(EXISTS "${remoteport_path}")
        file(TO_CMAKE_PATH "${remoteport_path}" remoteport_norm)
        list(APPEND preserve_paths "${remoteport_norm}")
      endif()

      foreach(entry IN LISTS output_entries)
        file(TO_CMAKE_PATH "${entry}" entry_norm)
        list(FIND preserve_paths "${entry_norm}" preserve_index)
        if(preserve_index EQUAL -1)
          file(REMOVE_RECURSE "${entry}")
        endif()
      endforeach()
    else()
      file(MAKE_DIRECTORY "${output_dir}")
    endif()

    message(STATUS "Extracting Dawn source archive to '${output_dir}'")
    execute_process(
      COMMAND "${CMAKE_COMMAND}" -E tar xf "${source_archive_path}" --format=zip
      WORKING_DIRECTORY "${output_dir}"
      RESULT_VARIABLE extract_result
    )

    if(NOT "${extract_result}" STREQUAL "0")
      message(FATAL_ERROR "Failed to extract Dawn source archive '${source_archive_path}'.")
    endif()

    file(WRITE "${stamp_file}" "${expected_stamp}")
  else()
    if(NOT EXISTS "${output_dir}")
      file(MAKE_DIRECTORY "${output_dir}")
    endif()
  endif()

  file(COPY "${remoteport_path}" DESTINATION "${output_dir}")

  set(remoteport_destination "${output_dir}/${remoteport_filename}")
  if(NOT EXISTS "${remoteport_destination}")
    message(FATAL_ERROR "Failed to place '${remoteport_filename}' into '${output_dir}'.")
  endif()

  message(STATUS "Dawn assets ready at '${output_dir}'.")
endfunction()

