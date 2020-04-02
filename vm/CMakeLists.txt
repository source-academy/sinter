cmake_minimum_required(VERSION 3.10)

set(SINTER_DEBUG_LEVEL 0 CACHE STRING "Debug level")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Defaulting to Debug build.")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE "Debug")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")

project(libsinter C)

add_library(sinter
  src/vm.c
  src/fault.c
  src/memory.c
  src/heap_mdestroy.c
  src/main.c
  src/debug.c
  src/inline.c
  src/primitives.c
)

target_compile_options(sinter
  PRIVATE -Wall -Wextra -Wswitch-enum -std=c11 -pedantic -Werror -fwrapv -g
  PRIVATE -DSINTER_DEBUG_LEVEL=${SINTER_DEBUG_LEVEL} $<$<BOOL:${SINTER_ABORT_ON_FAULT}>:-DSINTER_ABORT_ON_FAULT>
  PRIVATE $<$<CONFIG:Debug>:-Og -DSINTER_DEBUG>
  PRIVATE $<$<CONFIG:Release>:-O2 -DNDEBUG>
)

target_include_directories(sinter
  PUBLIC include
)

target_link_libraries(sinter m)
