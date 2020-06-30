#!/bin/bash

set -o pipefail

runner="$1"
in_file="$2.svm"
out_file="$2.out"

"$runner" "$in_file" 2>&1 | sed -e 's/0x[0-9A-F]\{5,\}/0xMEMADDR/ig' | diff -u "$out_file" -
