file(
  GLOB_RECURSE TEST_SOURCE
  "[a-z0-9]" *.hpp
  "[a-z0-9]" *.cpp
)

add_executable(
  qcc-test
  ${TEST_SOURCE}
)

target_include_directories(
  qcc-test PRIVATE
  ${CMAKE_SOURCE_DIR}/src/qcc
  ${CMAKE_SOURCE_DIR}/src/test
)

target_link_libraries(
  qcc-test PRIVATE
  qcc
  gtest
)

target_compile_definitions(
  qcc-test PRIVATE
  QCC_TEST_PATH="${CMAKE_SOURCE_DIR}/test/"
)
	
set_target_properties(
  qcc-test PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED YES
  LINKER_LANGUAGE CXX
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
