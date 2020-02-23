# Sinter VM C implementation

This is the "main" implementation of the Sinter VM, written in C11.

## Configuration

Some configuration is available via compiler defines.

- `SINTER_HEAP_SIZE`: size in bytes of the statically-allocated heap; defaults
to `0x10000` i.e. 64 KB
- `SINTER_STACK_ENTRIES`: size in stack entries of the statically-allocated
stack; defaults to `0x200` i.e. 512
- `SINTER_USE_SINGLE`, `SINTER_USE_DOUBLE`: whether to use single-precision or
double-precision float for numbers; defaults to the former
- `SINTER_SEATBELTS_OFF`: whether to disable certain safety checks in the
runtime e.g. stack over/underflow checks; defaults to unset (i.e. safety checks
are performed)
