cmake_minimum_required(VERSION 3.10)

set(SINTER_DEBUG_LOGLEVEL 0 CACHE STRING "Debug level")
set(SINTER_STATIC_HEAP 1 CACHE STRING "Enable static heap")
set(SINTER_TEST_SHORT_DOUBLE 0 CACHE STRING "Test short double workaround")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Defaulting to Debug build.")
  set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

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
  PRIVATE $<$<CONFIG:Debug>:$<IF:$<BOOL:${SINTER_NUMERIC_OPT}>,-O0,-Og>>
  PRIVATE $<$<CONFIG:Release>:-O2 -DNDEBUG>
  PUBLIC $<$<OR:$<CONFIG:Debug>,$<BOOL:${SINTER_DEBUG}>>:-DSINTER_DEBUG>
  PUBLIC $<$<BOOL:${SINTER_STATIC_HEAP}>:-DSINTER_STATIC_HEAP>
  PUBLIC -DSINTER_DEBUG_LOGLEVEL=${SINTER_DEBUG_LOGLEVEL}
  PUBLIC $<$<BOOL:${SINTER_DEBUG_ABORT_ON_FAULT}>:-DSINTER_DEBUG_ABORT_ON_FAULT>
  PUBLIC $<$<BOOL:${SINTER_DEBUG_MEMORY_CHECK}>:-DSINTER_DEBUG_MEMORY_CHECK>
  PUBLIC $<$<BOOL:${SINTER_DISABLE_CHECKS}>:-DSINTER_DISABLE_CHECKS>
  PUBLIC $<$<BOOL:${SINTER_TEST_SHORT_DOUBLE}>:-DSINTER_TEST_SHORT_DOUBLE>
  PUBLIC $<$<BOOL:${SINTER_COVERAGE}>:--coverage -fno-inline -fno-inline-small-functions -fno-default-inline>
)

if(DEFINED SINTER_HEAP_SIZE)
  target_compile_options(sinter PUBLIC -DSINTER_HEAP_SIZE=${SINTER_HEAP_SIZE})
  message(STATUS "Setting SINTER_HEAP_SIZE to ${SINTER_HEAP_SIZE}")
endif()

if(DEFINED SINTER_STACK_ENTRIES)
  target_compile_options(sinter PUBLIC -DSINTER_STACK_ENTRIES=${SINTER_STACK_ENTRIES})
  message(STATUS "Setting SINTER_STACK_ENTRIES to ${SINTER_STACK_ENTRIES}")
endif()

target_link_options(sinter
  PUBLIC $<$<BOOL:${SINTER_COVERAGE}>:--coverage>
)

target_include_directories(sinter
  PUBLIC include
)

target_link_libraries(sinter m)
