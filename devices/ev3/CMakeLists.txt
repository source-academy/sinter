cmake_minimum_required(VERSION 3.13)

set(CMAKE_BUILD_TYPE Release)
set(SINTER_DEBUG 0)
set(SINTER_DEBUG_LOGLEVEL 0)
set(SINTER_STATIC_HEAP 1)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

project(sinter-ev3 C)

add_subdirectory(../../vm libsinter)

add_executable(sinter-ev3
  ../../runner/src/runner.c
  ../../runner/src/display_object_result.c
  src/ev3_functions.c
)

target_compile_options(sinter-ev3
  PRIVATE -Wall -Wextra -Wswitch-enum -std=c11 -pedantic -Werror -fwrapv -g -O2
  PRIVATE -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE
)

target_link_libraries(sinter-ev3 sinter)
