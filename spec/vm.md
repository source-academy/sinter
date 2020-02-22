# Sinter VM

## General definitions

- A fault is an unrecoverable execution error. The fault handling mechanism is
implementation-defined; an implementation may print an error to the console,
for example.
- For all instructions below: Attempting to pop off an empty stack results in a
fault. Attempting to push a value to a full stack results in a fault.

Note: this bytecode format was written with the ESP32 in mind, which allows
byte-aligned access on the data bus. If porting this bytecode format to other
microcontrollers, consider aligning instructions and operands to some suitable
alignment e.g. 4-byte alignment.

### Values

The VM recognises 7 distinct types of values:

- `undefined`, which is a singleton
- `null`, which is also a singleton
- booleans, which is a doubleton; either `true` or `false`
- numbers, which are IEEE-754 single-precision 32-bit binary format values
- strings
- arrays
- functions

### Arrays

Arrays behave as they do in JavaScript:

- Access to an index that has been assigned returns the last-assigned value, as
expected.
- Access to an index that has not been assigned returns `undefined`.
- The length of an array is the value of the highest-assigned index, plus one.

### Functions

Functions are essentially a tuple of four values:

- The maximum stack size of the function
- The number of locals to allocate in the function's environment
- The address of the first instruction of the function
- The environment that will be the parent environment of the function's
environment whenever the function is called

### Stack and environment

Each function call creates a new stack and environment.

The environment stores:

- The locals
- The arguments passed to the function

The stack is used to pass operands to instructions. Each function call's stack
is independent of all other function calls' stacks. As mentioned above,
attempting to pop off an empty stack results in a fault. Attempting to push a
value to a full stack results in a fault.

## Instructions

- All operands are in target device endianness.
- Instructions are byte-aligned.
- Unless otherwise stated, instructions are one octet long.
- We use the integer and float type names from Rust to denote operand types
below.
  - E.g. `u8` refers to an 8-bit unsigned integer; `i32` refers to a 32-bit
  signed integer; `f32` refers to a 32-bit (single-precision) floating point.
- An `address` is a 32-bit unsigned integer `u32` that refers to an offset from
the start of the program.
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

Format: `0x0A`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are numbers, pushes a number equal to `a - b` onto the stack.

Otherwise, a fault occurs.

### `mul`: multiply

Format: `0x0B`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are numbers, pushes a number equal to `a * b` onto the stack.

Otherwise, a fault occurs.

### `div`: divide

Format: `0x0C`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are numbers, pushes a number equal to `a / b` onto the stack.

Otherwise, a fault occurs.

### `mod`: modulo

Format: `0x0D`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are numbers, pushes a number equal to `a % b` onto the stack.

Otherwise, a fault occurs.

### `not`: not

Format: `0x0E`

Pops `a` off the stack.

If `a` is a boolean, pushes its negation onto the stack.

Otherwise, a fault occurs.

### `lt`: less than

Format: `0x0F`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are both numbers or both strings, pushes a boolean equal to `a <
b` onto the stack, with the obvious meaning if both operands are numbers, and
comparing the operands by lexicographical order if they are strings.

Otherwise, a fault occurs.

### `gt`: greater than

Format: `0x10`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are both numbers or both strings, pushes a boolean equal to `a >
b` onto the stack, with the obvious meaning if both operands are numbers, and
comparing the operands by lexicographical order if they are strings.

Otherwise, a fault occurs.

### `le`: less than or equal to

Format: `0x11`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are both numbers or both strings, pushes a boolean equal to `a
<= b` onto the stack, with the obvious meaning if both operands are numbers,
and comparing the operands by lexicographical order if they are strings.

Otherwise, a fault occurs.

### `ge`: greater than or equal to

Format: `0x12`

Pops `b` off the stack, then pops `a` off the stack.

If `b` and `a` are both numbers or both strings, pushes a boolean equal to `a
>= b` onto the stack, with the obvious meaning if both operands are numbers,
and comparing the operands by lexicographical order if they are strings.

Otherwise, a fault occurs.

### `eq`: equal

Format: `0x13`

Pops `b` off the stack, then pops `a` off the stack.

If `a` and `b` are of different types, pushes `false` onto the stack.

If `a` and `b` are both `undefined` or both `null`, pushes `true` onto the
stack.

If `a` and `b` are both booleans, both numbers or both strings, pushes `true`
onto the stack if they have the same value, otherwise pushes `false` onto the
stack.

If `a` and `b` are both functions or both arrays, pushes `true` onto the stack
if they are referentially equal i.e. they refer to the exact same array or
function object, otherwise pushes `false` onto the stack.

The above cases are exhaustive.

### `new.f`: create function

Format: `0x14 <maxstack: u8> <framesize: u8> <address>` (7 octets)

Pushes a new function object onto the stack, with the given maximum stack size
`maxstack`, the given environment frame size `framesize`, and the first
instruction at `address`. The parent of the environment created when the new
function is called will be the current environment.

### `new.v`: create array (vector)

Format: `0x15`

Pushes a new empty array onto the stack.

### `ld.e`: load from current environment

Format: `0x16 <index: u8>` (2 octets)

Pushes the value at index `index` in the current environment onto the stack.

### `st.e`: store in current environment

Format: `0x17 <index: u8>` (2 octets)

Pops `a` off the stack, and stores `a` into index `index` in the current
environment.

### `ld.e.p`: load from environment

Format: `0x18 <index: u8> <envindex: u8>` (3 octets)

Pushes the value at index `index` in the `envindex`th parent of the current
environment onto the stack.

If `envindex` is `0`, this is equivalent to `ld.e`.

### `st.e.p`: store in environment

Format: `0x19 <index: u8> <envindex: u8>` (3 octets)

Pops `a` off the stack, and stores `a` into index `index` in the `envindex`th
parent of the current environment.

If `envindex` is `0`, this is equivalent to `st.e`.

### `ld.a`: load argument from current environment

Format: `0x1A <index: u8>` (2 octets)

Pushes the index `index` argument in the current environment onto the stack.

### `st.a`: store argument in current environment

Format: `0x1B <index: u8>` (2 octets)

Pops `a` off the stack, and stores `a` into the index `index` argument in the
current environment.

### `ld.a.p`: load argument

Format: `0x1C <index: u8> <envindex: u8>` (3 octets)

Pushes the index `index` argument in the `envindex`th parent of the current
environment onto the stack.

If `envindex` is `0`, this is equivalent to `ld.a`.

### `st.a.p`: store argument

Format: `0x1D <index: u8> <envindex: u8>` (3 octets)

Pops `a` off the stack, and stores `a` into the index `index` argument in the
`envindex`th parent of the current environment.

If `envindex` is `0`, this is equivalent to `st.a`.

### `ld.v`: load from array (vector)

Format: `0x1E`

Pops `index` off the stack, then pops `array` off the stack.

If `index` is a non-negative integer, pushes the value at index `index` in
`array` onto the stack.

Otherwise, a fault occurs.

### `st.v`: store in array (vector)

Format: `0x1F`

Pops `value` off the stack, pops `index` off the stack, then pops `array` off
the stack.

If `index` is a non-negative integer, stores `value` into index `index` in
`array`.

Otherwise, a fault occurs.

### `br.t`: branch if true

Format: `0x20 <offset>`

Pops `condition` off the stack.

If `condition` is a boolean and is true, skips `offset` bytes starting from
after the current instruction. That is, `br.t 0` is a no-op.

If `condition` is a boolean and is false, does nothing.

Otherwise, a fault occurs.

### `br`: branch

Format: `0x21 <offset>`

Skips `offset` bytes starting from after the current instruction. That is, `br
0` is a no-op.

### `jmp`: jump

Format: `0x22 <address>`

Jumps to `address`. Note: the current environment and stack are unchanged.

### `call`: call function

Format: `0x23 <numargs: u8>`

Pops the arguments to be passed to the function off the stack in reverse order,
followed by the function itself. That is, pop the last argument, followed by
the second last, and so on, until the first argument, and then the function.

Calls the function, and then pushes the return value of the function onto the
stack.

### `call.t`: tail call function

Format: `0x24 <numargs: u8>`

Pops the arguments to be passed to the function off the stack in reverse order,
followed by the function itself. That is, pop the last argument, followed by
the second last, and so on, until the first argument, and then the function.

Calls the function. The return value of the callee function will become the
return value of the current function, and execution returns to the caller of
the current function.

### `call.p`: call primitive function

Format: `0x25 <id: u8> <numargs: u8>`

Pops the arguments to be passed to the function off the stack in reverse order.
That is, pop the last argument, followed by the second last, and so on, until
the first argument.

Calls the primitive function with the given `id`, and then pushes the return
value of the function onto the stack.

### `call.t.p`: tail call primitive function

Format: `0x26 <id: u8> <numargs: u8>`

Pops the arguments to be passed to the function off the stack in reverse order.
That is, pop the last argument, followed by the second last, and so on, until
the first argument.

Calls the primitive function with the given `id`. The return value of the
primitive function will become the return value of the current function, and
execution returns to the caller of the current function.

### `call.v`: call VM-internal function/native function

Format: `0x27 <id: u8> <numargs: u8>`

Pops the arguments to be passed to the function off the stack in reverse order.
That is, pop the last argument, followed by the second last, and so on, until
the first argument.

Calls the VM-internal function with the given `id`, and then pushes the return
value of the function onto the stack.

### `call.t.v`: tail call VM-internal function/native function

Format: `0x28 <id: u8> <numargs: u8>`

Pops the arguments to be passed to the function off the stack in reverse order.
That is, pop the last argument, followed by the second last, and so on, until
the first argument.

Calls the VM-internal function with the given `id`. The return value of the
VM-internal function will become the return value of the current function, and
execution returns to the caller of the current function.

### `ret`: return

Format: `0x29`

Pops `retval` off the stack. Makes `retval` the return value of the current
function, and returns execution to the caller of the current function.

### `ret.u`: return undefined

Format: `0x2A`

Makes `undefined` the return value of the current function, and returns
execution to the caller of the current function.
