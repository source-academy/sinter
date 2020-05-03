# Sinter VM

For implementation details, see [here](docs/impl.md).

## Usage notes

Sinter deviates from the normal Source in a few ways:

- Numbers are single-precision floating points. This means that
  `16777216 + 1 === 16777216`.
- The following primitives are not supported:
  - list_to_string
  - parse_int
  - runtime
  - prompt
  - stringify

Usage recommendations:

- Treat arrays like C arrays, rather than JavaScript arrays (which are actually
  maps). Sinter does not (yet) have optimisations for sparse arrays.

## Use it on a device

Sinter is a C library. For examples on how to use Sinter, see [the CLI
runner](../runner/src/runner.c), [the Arduino sketch
example](../devices/arduino/arduino.ino), or the [ESP32
example](../devices/esp32/src/main.c).

To create an Arduino library zip, run the script
[`make_arduino_lib.sh`](../make_arduino_lib.sh). You can configure the Arduino
library by unzipping the zip and modifying
[`sinter_config.h`](include/sinter_config.h).

## Build locally

We use the CMake build system. Note: a compiler that supports C11 is _required_.
This excludes MSVC.

```
# cd into the root of the repository
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DSINTER_DEBUG_LOGLEVEL=2 -DSINTER_DEBUG_MEMORY_CHECK=1
make -j8
make test
runner/runner ../test_programs/prim_display.svm
```

### CMake configuration

Some configuration is available via CMake defines:

- `CMAKE_BUILD_TYPE`: controls the build type; defaults to `Debug`

  - `Debug`: assertions are enabled; some extra checks are enabled; `-Og` optimisation level
  - `Release`: assertions are disabled; `-O2` optimisation level

- `SINTER_HEAP_SIZE`: size in bytes of the statically-allocated heap; defaults
  to `0x10000` i.e. 64 KB

- `SINTER_STACK_ENTRIES`: size in stack entries of the statically-allocated
  stack; defaults to `0x200` i.e. 512

- `SINTER_DISABLE_CHECKS`: if `1`, disables certain safety checks in the runtime
  e.g. stack over/underflow checks; defaults to unset (i.e. safety checks are
  performed)

- `SINTER_DEBUG_LOGLEVEL`: controls the debug output level; defaults to `0`

  - `0`: all debug output is disabled.
  - `1`: prints reasons for most faults/errors
  - `2`: traces every instruction executed and every push on/pop off the stack

  This is available regardless of `CMAKE_BUILD_TYPE`.

  When deploying on an actual microcontroller, you will likely want to use `0`.
  `1` and `2` requires some implementation of `fprintf` and `stderr`. (This may
  be relaxed in future so the library consumer can provide a logging function
  instead.)

- `SINTER_DEBUG_ABORT_ON_FAULT`: if `1`, raises an assertion failure when a
  fault occurs. (Intended for use when debugging under e.g. GDB.) Defaults to
  unset.

- `SINTER_DEBUG_MEMORY_CHECK`: if `1`, does _a lot_ of checks at every
  instruction to verify the correctness of the heap linked list, freelist,
  stack, and reference counting. Note: this slows down execution severely.
  Defaults to unset.
