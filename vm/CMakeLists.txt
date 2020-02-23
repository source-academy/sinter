cmake_minimum_required(VERSION 3.10)

project(libsinter C)

add_library(sinter
  src/vm.c
  src/fault.c
  src/memory.c
)

target_compile_options(sinter
  PRIVATE -Wall -Wextra -std=c11 -pedantic -Werror
)

target_include_directories(sinter
  PUBLIC include
)
