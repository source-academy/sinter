# Sinter VM

- A fault is an unrecoverable execution error. The fault handling mechanism is
implementation-defined; an implementation may print an error to the console,
for example.
- For all instructions below: Attempting to pop off an empty stack results in a
fault. Attempting to push a value to a full stack results in a fault.

*TODO: describe stack, environment, as well as possible values (number, string,
array, function, undefined, null)*

Note: this bytecode format was written with the ESP32 in mind, which allows
byte-aligned access on the data bus. If porting this bytecode format to other
microcontrollers, consider aligning instructions and operands to some suitable
alignment e.g. 4-byte alignment.

## Instructions

- All operands are in target device endianness.
- Instructions are byte-aligned.
- Unless otherwise stated, instructions are one octet long.
- We use the integer and float type names from Rust to denote operand types below.
  - E.g. `u8` refers to an 8-bit unsigned integer; `i32` refers to a 32-bit
  signed integer; `f32` refers to a 32-bit (single-precision) floating point.
- An `address` is a 32-bit unsigned integer `u32` that refers to an offset from the
start of the program.
- An `offset` is a 32-bit signed integer `u8` that refers to an offset from the
start of the _next_ instruction.

### `nop`: no-op

Format: `0x00`

Does nothing.

### `ldc.i`: load constant integer

Format: `0x01 <u32>` (5 octets)

Pushes a number, whose value is equal to the operand, onto the stack.

### `ldc.f`: load constant float

Format: `0x02 <f32>` (5 octets)

Pushes a number, whose value is equal to the operand, onto the stack.

### `ldc.b.0`: load constant false

Format: `0x03`

Pushes the boolean `false` onto the stack.

### `ldc.b.1`: load constant true

Format: `0x04`

Pushes the boolean `true` onto the stack.

### `ldc.u`: load constant undefined

Format: `0x05`

Pushes `undefined` onto the stack.

### `ldc.n`: load constant null

Format: `0x06`

Pushes `null` onto the stack.

### `ldc.s`: load constant string

Format: `0x07 <address>` (5 octets)

Pushes the string at the given address onto the stack.

### `pop`: pop from stack

Format: `0x08`

Pops the topmost value off the stack.

### `add`: add

Format: `0x09`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are strings, pushes a string equal to the concatenation of `a`
and `b`, in that order, onto the stack.

If `b` and `a` are numbers, pushes their sum onto the stack.

Otherwise, a fault occurs.

### `sub`: subtract

Format: `0x10`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are numbers, pushes a number equal to `a - b` onto the stack.

Otherwise, a fault occurs.

### `mul`: multiply

Format: `0x11`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are numbers, pushes a number equal to `a * b` onto the stack.

Otherwise, a fault occurs.

### `div`: divide

Format: `0x12`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are numbers, pushes a number equal to `a / b` onto the stack.

Otherwise, a fault occurs.

### `mod`: modulo

Format: `0x13`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are numbers, pushes a number equal to `a % b` onto the stack.

Otherwise, a fault occurs.

### `not`: not

Format: `0x14`

Pops `a` off the stack.

If `a` is a boolean, pushes its negation onto the stack.

Otherwise, a fault occurs.

### `lt`: less than

Format: `0x15`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are both numbers or both strings, pushes a boolean equal to `a < b` onto the stack, with the obvious meaning if both operands are numbers, and comparing the operands by lexicographical order if they are strings.

Otherwise, a fault occurs.

### `gt`: greater than

Format: `0x16`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are both numbers or both strings, pushes a boolean equal to `a > b` onto the stack, with the obvious meaning if both operands are numbers, and comparing the operands by lexicographical order if they are strings.

Otherwise, a fault occurs.

### `le`: less than or equal to

Format: `0x17`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are both numbers or both strings, pushes a boolean equal to `a <= b` onto the stack, with the obvious meaning if both operands are numbers, and comparing the operands by lexicographical order if they are strings.

Otherwise, a fault occurs.

### `ge`: greater than or equal to

Format: `0x18`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are both numbers or both strings, pushes a boolean equal to `a >= b` onto the stack, with the obvious meaning if both operands are numbers, and comparing the operands by lexicographical order if they are strings.

Otherwise, a fault occurs.

### `eq`: equal

Format: `0x19`

Pops `b` off the stack, then pops `a` off the stack.

If `a` and `b` are of different types, pushes `false` onto the stack.

If `a` and `b` are both `undefined` or both `null`, pushes `true` onto the stack.

If `a` and `b` are both numbers or both strings, pushes `true` onto the stack if they have the same value, otherwise pushes `false` onto the stack.

If `a` and `b` are both functions or both arrays, pushes `true` onto the stack if they are referentially equal i.e. they refer to the exact same array or function object, otherwise pushes `false` onto the stack.

The above cases are exhaustive.

### `new.f`: create function

Format: `0x20 <maxstack: u8> <framesize: u8> <address>` (7 octets)

Pushes a new function object onto the stack, with the given maximum stack size `maxstack`,
the given environment frame size `framesize`, and the first instruction at `address`.

### `new.v`: create array (vector)

Format: `0x21`

Pushes a new empty array onto the stack.

### `ld.e`: load from current environment

Format: `0x22 <index: u8>` (2 octets)

Pushes the value at index `index` in the current environment onto the stack.

### `st.e`: store in current environment

Format: `0x23 <index: u8>` (2 octets)

Pops `a` off the stack, and stores `a` into index `index` in the current environment.

### `ld.e.p`: load from environment

Format: `0x24 <index: u8> <envindex: u8>` (3 octets)

Pushes the value at index `index` in the `envindex`th parent of the current
environment onto the stack.

If `envindex` is `0`, this is equivalent to `ld.e`.

### `st.e.p`: store in environment

Format: `0x25 <index: u8> <envindex: u8>` (3 octets)

Pops `a` off the stack, and stores `a` into index `index` in the `envindex`th
parent of the current environment.

If `envindex` is `0`, this is equivalent to `st.e`.

### `ld.a`: load argument
### `st.a`: store argument
### `ld.v`: load from array (vector)
### `st.v`: store in array (vector)
### `br.t`: branch if true
### `br`: branch
### `jmp`: jump
### `call`: call function
### `call.t`: tail call function
### `call.p`: call primitive function
### `call.v`: call VM-internal function/native function
### `ret`: return
### `ret.u`: return undefined
