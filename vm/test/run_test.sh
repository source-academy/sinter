#!/bin/bash

set -o pipefail

runner="$1"
in_file="$2.svm"
out_file="$2.out"

"$runner" "$in_file" | diff -u "$out_file" -
