cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 17)

if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

set(LIBRARY_NAME foxglove_core)
project(${LIBRARY_NAME} LANGUAGES C CXX)

set(PACKAGE_DIR "${CMAKE_BINARY_DIR}/${LIBRARY_NAME}_packages")

set(LIBVLC_NIGHTLY_VERSION "20220120-0425")
set(LIBVLC_VERSION "4.0.0-dev")
set(LIBVLC_SHA256 "8a6377aa9c6468833b90acb46fa07b63f190080c7cda052a151d7a95aec9a2ad")
set(LIBVLC_URL "https://artifacts.videolan.org/vlc/nightly-win64/${LIBVLC_NIGHTLY_VERSION}/vlc-${LIBVLC_VERSION}-win64-7beb0605.7z")
set(LIBVLC_ARCHIVE "${PACKAGE_DIR}/libvlc-${LIBVLC_NIGHTLY_VERSION}.7z")
set(LIBVLC_ARCHIVE_ROOT "vlc-${LIBVLC_VERSION}/")
set(LIBVLC_SOURCE "${PACKAGE_DIR}/vlc-${LIBVLC_VERSION}")

set(LIBVLCPP_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libvlcpp")

add_custom_target(LIBVLC_EXTRACT ALL)
get_directory_property(_HAS_PARENT_DIRECTORY PARENT_DIRECTORY)
if(NOT _HAS_PARENT_DIRECTORY)
  # Indicates that this is a top-level CMake project.
  set(IS_STANDALONE TRUE)
endif()


if (NOT EXISTS "${LIBVLC_ARCHIVE}")
  file(DOWNLOAD "${LIBVLC_URL}" "${LIBVLC_ARCHIVE}")
endif()

file(SHA256 ${LIBVLC_ARCHIVE} LIBVLC_DL_HASH)

if (NOT LIBVLC_DL_HASH STREQUAL LIBVLC_SHA256)
  message(FATAL_ERROR "Integrity check for ${LIBVLC_ARCHIVE} failed")
endif()

if (NOT EXISTS "${LIBVLC_SOURCE}")
  if(NOT WIN32)
    # Headers only.
    set(LIBVLC_FILES sdk)
  else()
    if(FOXGLOVE_VLC_PLUGINS)
      list(TRANSFORM FOXGLOVE_VLC_PLUGINS PREPEND "plugins/")
    else()
      # Extract the whole plugins directory
      # by default
      set(FOXGLOVE_VLC_PLUGINS plugins)
    endif()

    set(LIBVLC_FILES
      sdk
      libvlc.dll
      libvlccore.dll
      ${FOXGLOVE_VLC_PLUGINS}
    )
  endif()

  list(TRANSFORM LIBVLC_FILES PREPEND ${LIBVLC_ARCHIVE_ROOT})

  file(MAKE_DIRECTORY ${PACKAGE_DIR})
  add_custom_command(
    TARGET LIBVLC_EXTRACT PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E tar xzf \"${LIBVLC_ARCHIVE}\" -- ${LIBVLC_FILES}
    WORKING_DIRECTORY "${PACKAGE_DIR}"
    DEPENDS "${LIBVLC_ARCHIVE}"
  )
endif()

add_library(${LIBRARY_NAME} STATIC
  events.cc
  globals.cc
  base/task_queue.cc
  vlc/vlc_environment.cc
  vlc/vlc_player.cc
  vlc/vlc_playlist.cc
)

add_dependencies(${LIBRARY_NAME} LIBVLC_EXTRACT)

set_target_properties(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${LIBRARY_NAME} PRIVATE
  # foxglove wrapper headers.
  "${CMAKE_CURRENT_SOURCE_DIR}"
  # libVLC++ headers.
  "${LIBVLCPP_SOURCE}"
  # libVLC headers.
  "${LIBVLC_SOURCE}/sdk/include"
  # Dart API headers.
  "${DARTAPI_SOURCE}"
)

target_include_directories(${LIBRARY_NAME} INTERFACE
  # libVLC++ headers.
  "${LIBVLCPP_SOURCE}"
  # libVLC headers.
  "${LIBVLC_SOURCE}/sdk/include"
)

# Windows
if(WIN32)
  set_target_properties(${LIBRARY_NAME} PROPERTIES CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  target_link_libraries(${LIBRARY_NAME} PRIVATE
    "${LIBVLC_SOURCE}/sdk/lib/libvlc.lib"
    "${LIBVLC_SOURCE}/sdk/lib/libvlccore.lib"
  )

  # Add generated shared library & libVLC DLLs.
  set(FOXGLOVE_CORE_LIBS
    # In case we decide to build this as a shared library
    #"$<TARGET_FILE:foxglove_core>"
    "${LIBVLC_SOURCE}/libvlc.dll"
    "${LIBVLC_SOURCE}/libvlccore.dll"
    "${LIBVLC_SOURCE}/plugins"
    PARENT_SCOPE
  )

# Linux
elseif(LINUX)
  # Build with -fPIC
  set_target_properties(${LIBRARY_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
  # Link against platform-provided libvlc
  target_link_libraries(${LIBRARY_NAME} PRIVATE
    "vlc"
  )
endif()

# If this is the top-level CMake project (e.g. on macOS where this is being run
# by a CocoaPods script phase) we "install" the library directly
if(IS_STANDALONE)
  install(TARGETS ${LIBRARY_NAME})
else()
  set(FOXGLOVE_CORE_PATH ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
endif()

