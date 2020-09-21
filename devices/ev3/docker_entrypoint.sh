#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath -s "${BASH_SOURCE[0]}")")"
cd "$SCRIPT_DIR"

mkdir -p build-ev3
cd build-ev3
cmake "$SCRIPT_DIR" -DCMAKE_TOOLCHAIN_FILE=/home/compiler/toolchain-armel.cmake -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
