# Sinter VM C implementation

This is the "main" implementation of the Sinter VM, written in C11.

## Build and configuration

We use the CMake build system. Note: a compiler that supports C11 is _required_. This excludes MSVC.

```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DSINTER_DEBUG_LEVEL=2
make -j8
make test
runner/runner ../test_programs/display.svm
```

Some configuration is available via CMake defines:

- `CMAKE_BUILD_TYPE`: controls the build type; defaults to `Debug`

  - `Debug`: assertions are enabled; `-Og` optimisation level
  - `Release`: assertions are disabled; `-O2` optimisation level

- `SINTER_HEAP_SIZE`: size in bytes of the statically-allocated heap; defaults
to `0x10000` i.e. 64 KB

- `SINTER_STACK_ENTRIES`: size in stack entries of the statically-allocated
stack; defaults to `0x200` i.e. 512

- `SINTER_SEATBELTS_OFF`: whether to disable certain safety checks in the
runtime e.g. stack over/underflow checks; defaults to unset (i.e. safety checks
are performed)

- `SINTER_DEBUG_LEVEL`: controls the debug output level; defaults to `0`

  - `0`: all debug output is disabled.
  - `1`: prints reasons for most faults/errors
  - `2`: traces every instruction executed and every push on/pop off the stack

  This is available regardless of `CMAKE_BUILD_TYPE`.

  When deploying on an actual microcontroller, you will likely want to use `0`. `1` and `2` requires some implementation of `fprintf` and `stderr`. (This may be relaxed in future so the library consumer can provide a logging function instead.)

- `SINTER_ABORT_ON_FAULT`: if `1`, raises an assertion failure when a fault occurs. (Intended for use when debugging under e.g. GDB.) Defaults to `0`.
