#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
TMPDIR="$(mktemp -d)"

mkdir -p "$TMPDIR/Sinter/src"
cp -rv "$DIR"/vm/src/* "$DIR"/vm/include/* "$TMPDIR/Sinter/src"
cp -v "$DIR/Arduino-library.properties" "$TMPDIR/Sinter/library.properties"
pushd "$TMPDIR"
zip -rv "$DIR/Sinter.zip" *
popd

rm -rf "$TMPDIR"
