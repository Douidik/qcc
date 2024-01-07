cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG master
)
FetchContent_Declare(
  gtest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG main
)

FetchContent_MakeAvailable(fmt)
FetchContent_MakeAvailable(gtest)

project(
  qcc
  DESCRIPTION "Experimental C-compiler"
  LANGUAGES CXX
)

project(
  qcc-cmd
  LANGUAGES CXX
)

project(
  qcc-test
  LANGUAGES CXX
)

add_subdirectory(src/qcc)
add_subdirectory(src/cmd)
add_subdirectory(src/test)

