file(
  GLOB_RECURSE CMD_SOURCE
  "[a-z0-9]" *.hpp
  "[a-z0-9]" *.cpp
)

add_executable(
  qcc-cmd
  ${CMD_SOURCE}
)

target_include_directories(
  qcc-cmd PRIVATE
  ${CMAKE_SOURCE_DIR}/src/qcc
  ${CMAKE_SOURCE_DIR}/src/cmd
)

target_link_libraries(
  qcc-cmd PRIVATE
  qcc
)

set_target_properties(
  qcc-cmd PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED YES
  LINKER_LANGUAGE CXX
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
