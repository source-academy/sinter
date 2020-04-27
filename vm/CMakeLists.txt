cmake_minimum_required(VERSION 3.10)

set(SINTER_DEBUG_LOGLEVEL 0 CACHE STRING "Debug level")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Defaulting to Debug build.")
  set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")

project(libsinter C)

add_library(sinter
  src/vm.c
  src/fault.c
  src/memory.c
  src/main.c
  src/debug.c
  src/debug_memorycheck.c
  src/inline.c
  src/primitives.c
)

target_compile_options(sinter
  PRIVATE -Wall -Wextra -Wswitch-enum -std=c11 -pedantic -Werror -fwrapv -g
  PRIVATE $<$<CONFIG:Debug>:-Og>
  PRIVATE $<$<CONFIG:Release>:-O2 -DNDEBUG>
  PUBLIC $<$<CONFIG:Debug>:-DSINTER_DEBUG>
  PUBLIC -DSINTER_STATIC_HEAP
  PUBLIC -DSINTER_DEBUG_LOGLEVEL=${SINTER_DEBUG_LOGLEVEL}
  PUBLIC $<$<BOOL:${SINTER_DEBUG_ABORT_ON_FAULT}>:-DSINTER_DEBUG_ABORT_ON_FAULT>
  PUBLIC $<$<BOOL:${SINTER_DEBUG_MEMORY_CHECK}>:-DSINTER_DEBUG_MEMORY_CHECK>
  PUBLIC $<$<BOOL:${SINTER_COVERAGE}>:--coverage -fno-inline -fno-inline-small-functions -fno-default-inline>
)

target_link_options(sinter
  PUBLIC $<$<BOOL:${SINTER_COVERAGE}>:--coverage>
)

target_include_directories(sinter
  PUBLIC include
)

target_link_libraries(sinter m)
