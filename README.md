# Sinter

Name etymology: <strong>S</strong>VM <strong>inter</strong>preter. (This isn't a direct Source interpreter though.)

This is an implementation of the Source language intended for microcontroller platforms like an Arduino.

In order to avoid having to do lexing and parsing on extremely low-power devices, we first compile Source to a VM designed to suit Source, and then interpret the VM bytecode.

## Directory layout

- `vm`: The actual VM library.
- `vm_test`: Some scripts to aid with CI testing.
- `runner`: A simple runner to run programs from the CLI.
- `test_programs`: SVML test programs that have been manually verified to be correct, as well as expected output for automated tests.

## Build

We use the CMake build system. Note: a compiler that supports C11 is _required_. This excludes MSVC.

```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DSINTER_DEBUG_LEVEL=2
make -j8
make test
runner/runner ../test_programs/display.svm
```

For configuration options, see the README in `vm`.

## Specifications

We follow the [Source VM specification](https://github.com/source-academy/js-slang/wiki/SVML-Specification) as in the js-slang wiki.
