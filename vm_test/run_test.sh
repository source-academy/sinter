#!/bin/bash

runner="$1"
in_file="$2.svm"
out_file="$2.out"

"$runner" "$in_file" | diff - "$out_file"
