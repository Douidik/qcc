file(
	GLOB_RECURSE QCC_SOURCE
	"[a-z0-9]" *.hpp
	"[a-z0-9]" *.cpp
)

add_library(
	qcc STATIC
	${QCC_SOURCE}
)

target_include_directories(
	qcc PRIVATE
	${CMAKE_SOURCE_DIR}/src/qcc
)

target_link_libraries(
	qcc PUBLIC
	fmt::fmt
)

set_target_properties(
  qcc PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED YES
  LINKER_LANGUAGE CXX
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

target_compile_options(
  qcc PUBLIC
  -Wno-conversion-null
)
