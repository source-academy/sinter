#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath -s "${BASH_SOURCE[0]}")")"
cd "$SCRIPT_DIR"
docker build -t sourceacademy/sinter-ev3-build .
docker run --rm -it -v "$(realpath "$SCRIPT_DIR/../..")":/src -w /src/devices/ev3 -u 0:0 --entrypoint /src/devices/ev3/.docker_entrypoint.sh sourceacademy/sinter-ev3-build
