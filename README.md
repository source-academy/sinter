# Sinter

[![Coverage Status](https://coveralls.io/repos/github/source-academy/sinter/badge.svg?branch=master)](https://coveralls.io/github/source-academy/sinter?branch=master)

Name etymology: <strong>S</strong>VML <strong>inter</strong>preter.

This is an implementation of the Source Virtual Machine Language intended for microcontroller platforms like an Arduino. We follow the [Source VM specification](https://github.com/source-academy/js-slang/wiki/SVML-Specification) as in the js-slang wiki.

Use this VM with the [reference compiler](https://github.com/source-academy/js-slang/blob/master/src/vm/svmc.ts).

For more information, such as build and usage instructions, see the [README in `vm`](vm/README.md).

## Directory layout

- `vm`: The actual VM library.
- `vm_test`: Some scripts to aid with CI testing.
- `runner`: A simple runner to run programs from the CLI.
- `test_programs`: SVML test programs that have been manually verified to be correct, as well as expected output for automated tests.
- `devices`: Some examples for using Sinter on various embedded platforms.
