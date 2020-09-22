#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath -s "${BASH_SOURCE[0]}")")"
cd "$SCRIPT_DIR"

sudo -u \#$(stat --printf="%u" "$SCRIPT_DIR/build.sh") -g \#$(stat --printf="%g" "$SCRIPT_DIR/build.sh") "$SCRIPT_DIR/.docker_build.sh"
