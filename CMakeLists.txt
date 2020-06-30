cmake_minimum_required(VERSION 3.10)

project(sinter C)

enable_testing()

add_subdirectory(vm)
add_subdirectory(runner)
add_subdirectory(vm/test)
