cmake_minimum_required(VERSION 3.10)

set(CMAKE_BUILD_TYPE Release)
set(SINTER_NUMERIC_OPT 1)
set(SINTER_DEBUG 1)
set(SINTER_DEBUG_MEMORY_CHECK 1)
set(SINTER_DEBUG_LOGLEVEL 2)
set(SINTER_STATIC_HEAP 0)

project(sinter-wasm C)

add_subdirectory(../../../vm libsinter)

add_executable(sinterwasm lib.c)

target_link_options(sinterwasm
  PRIVATE "SHELL:-s WASM=1" "SHELL:-s ALLOW_MEMORY_GROWTH=1"
  PRIVATE "SHELL:-s MODULARIZE=1" "SHELL:-s EXPORT_NAME=sinterwasm"
  PRIVATE "SHELL:-s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\"]'"
)

target_link_libraries(sinterwasm sinter)

