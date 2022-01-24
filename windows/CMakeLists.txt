cmake_minimum_required(VERSION 3.14)
set(PROJECT_NAME "foxglove")
project(${PROJECT_NAME} LANGUAGES CXX)

add_subdirectory(../core "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_core")

# This value is used when generating builds using this plugin, so it must
# not be changed
set(PLUGIN_NAME "foxglove_plugin")

add_library(${PLUGIN_NAME} SHARED
  "foxglove_plugin.cpp"
  "method_channel_handler.cc"
  "player_bridge.cc"
)
apply_standard_settings(${PLUGIN_NAME})
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  CXX_STANDARD 17
)
 
target_compile_features(${PLUGIN_NAME} PRIVATE
  cxx_std_17
)

target_compile_definitions(${PLUGIN_NAME} PRIVATE
  FLUTTER_PLUGIN_IMPL
)

target_include_directories(${PLUGIN_NAME} INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(${PLUGIN_NAME} PRIVATE
  "${FOXGLOVE_CORE_PATH}"
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
  flutter
  flutter_wrapper_plugin
  foxglove_core
)

# We're currently building the core library as a static
# library. So we need to force exporting all symbols.
#target_link_options("${PLUGIN_NAME}" PRIVATE "/WHOLEARCHIVE:foxglove_core")

set(
  foxglove_bundled_libraries
  # the core library dependencies
  ${FOXGLOVE_CORE_LIBS}
  PARENT_SCOPE
)