cmake_minimum_required(VERSION 3.10)

set_property(CACHE SINTER_DEBUG_LEVEL PROPERTY STRINGS 0 1 2)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Defaulting to Debug build.")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE "Debug")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

if(NOT SINTER_DEBUG_LEVEL)
  set_property(CACHE SINTER_DEBUG_LEVEL PROPERTY VALUE 0)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")

project(libsinter C)

add_library(sinter
  src/vm.c
  src/fault.c
  src/memory.c
)

target_compile_options(sinter
  PRIVATE -Wall -Wextra -Wswitch-enum -std=c11 -pedantic -Werror -fwrapv -g -DSINTER_DEBUG_LEVEL=${SINTER_DEBUG_LEVEL}
  PRIVATE $<$<CONFIG:Debug>:-Og -DSINTER_DEBUG>
  PRIVATE $<$<CONFIG:Release>:-O2 -DNDEBUG>
)

target_include_directories(sinter
  PUBLIC include
)

target_link_libraries(sinter m)
