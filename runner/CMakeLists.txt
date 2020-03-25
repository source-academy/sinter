cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Defaulting to DebugLog build.")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE "DebugLog")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "DebugLog" "ReleaseLog")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")

project(runner C)

add_executable(runner
  src/runner.c
)

target_compile_options(runner
  PRIVATE -Wall -Wextra -Wswitch-enum -std=c11 -pedantic -Werror -fwrapv -g
  PRIVATE $<$<CONFIG:DebugLog>:-Og>
  PRIVATE $<$<CONFIG:ReleaseLog>:-O2>
)

target_link_libraries(runner sinter)