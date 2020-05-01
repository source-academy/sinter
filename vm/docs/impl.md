# Sinter implementation

## Overall source organisation

- [Main loop](../src/vm.c)
- [NaNbox](../include/sinter/nanbox.h)
- [Heap and memory management](../include/sinter/heap.h)
- [Heap objects](../include/sinter/heap_obj.h)
- [Stack](../include/sinter/stack.h)
- [Opcodes](../include/sinter/opcode.h)
- [Executable format](../include/sinter/program.h)
- [Entry point](../src/main.c)

Many functions are defined inline in header files. This is to give the compiler
the best chance at doing inlining and/or optimisations, to reduce the height
of the C stack where possible.

In case you are unfamiliar with C `inline`: An `inline` definition does not
create an externally-visible symbol. A C compiler may or may not inline a function
depending on whether it thinks it will be beneficial to do so; if it does not,
then it will emit a call to the symbol. In Sinter, we use `SINTER_INLINE` to make
functions inline; in [`inline.c`](../src/inline.c) we define away `SINTER_INLINE`
which causes an externally-visible symbol to be emitted. Unused functions are
typically removed at link time, if the appropriate linker flag is provided. (This
is typically the case for embedded platforms.)

## The heap

The Sinter heap is a doubly linked list of heap blocks. That is, each heap block
has a "pointer" to the next block (by virtue of its size field), and a pointer to
the previous block.

Each heap block header has a size, reference count, and type. Note that for
simplicity, the size includes the header.

The Sinter heap starts off in `siheap_init` as a single free block with size equal
to the entire heap. As allocations are made via `siheap_malloc`, we split off the
free block as needed. When allocations are freed in `siheap_mfree`, we merge with
(physically) adjacent free blocks, if they exist.

We also track free blocks in a separate doubly-linked list of free blocks only.
The doubly-linked list is stored within the data area of each free block.

The free block selection algorithm used currently is first-fit.

## Memory management

Memory management in Sinter is done using a combination of reference-counting
and mark-sweep collection. Reference-counting handles most of the cases; mark-sweep
only runs when the heap is full, and takes care of reference cycles.

Correctness of reference-counting is checked after every instruction in debug builds.
We walk the entire heap and stack, and count every reference, and check that the
live reference count tallies with the actual number of references.
See [`debug_memorycheck.c`](../src/debug_memorycheck.c).

TODO: Document reference-counting convention

## The stack

Sinter uses a single array to store all SVML function operand stacks. We detect
stack overflows or underflows within each function's stack with a stack bottom
and stack limit pointer that is re-set at each function call.

The calling function's stack limits are stored in a heap object, a pointer to which
is pushed right after the stack of the callee (and before that of the caller).

All entries on the stack are _NaNboxes_.

## NaNboxes

Sinter represents all values using _NaNboxes_. A detailed explanation of Sinter's
NaNboxing scheme is in the [comment in `nanbox.h`](../include/sinter/nanbox.h).

In IEEE-754 floating point representations, a NaN is any value where all bits of
the exponent are set, and the mantissa is nonzero. Since there are 23 or 52 bits
of mantissa in a float and double, respectively, that means there are 24 or 53 bits
of space in which other values can be stuffed.

NaNboxing is made possible by the fact that on many systems, float operations that
result in a NaN produce only a single "canonical NaN" value which is the NaN with
only the highest mantissa bit set.

## Types in Sinter

In Sinter, the following types are stored in NaNboxes:

- empty
- undefined
- null
- booleans
- small integers (-0x100000 &le; x &le; 0xFFFFF)
- floats
- pointers
- internal function references

The following types are stored on the heap, and represented as pointers:

- environments
- function stack information (as mentioned above)
- string constant references
- string pairs
- strings
- arrays
- SVML function objects (closures)
- internal continuation functions

All types either always live in a NaNbox, or always live on the heap. This simplifies
the implementation.

### Numbers

Sinter numbers are *single-precision `float`s*. This is a deviation from JavaScript
and Source, but is done in order to maintain good performance on embedded devices,
which typically do not support double-precision floating point in hardware.

Sinter tries to store small integers as integers where possible, as a further
optimisation.

### Strings

Strings are represented as either string constant references, string pairs,
or strings (i.e. the whole string is in the heap).

String pairs are used to represent the result of concatenation, and are only
flattened into a string when its result is needed (i.e. when the string pair is
an operand of a comparison, or it is returned from the top-level). This is the
only way to create a flattened string heap object.

Note that `display`ing a string does not flatten it; we simply print each part
in succession.

### Functions

There are three types of "function values"&mdash;SVML function objects (closures),
internal function references, and internal continuation functions. These are
all exposed as functions (i.e. `is_function` returns true for them).

All function values that refer to a function in the SVML program are SVML function
objects. As expected, they are simply a tuple of the code pointer and parent
environment pointer. (We also track the arity of the function.)

When primitive functions are passed as a value, an internal function reference
is created. This is simply a tuple of the type (primitive or VM-internal) and
the function index.

Internal continuation functions are used in the implementation of the stream
library.

## Primitives and VM-internal functions

Sinter implements most of the 92 Source primitive functions, including the list
and stream libraries in C, in the hopes of better performance. (This is yet to be
verified.)

The primitive functions are implemented in [`primitives.c`](../src/primitives.c).

The stream library is implemented using "internal continuations". In essence,
these are heap objects that store a C function pointer and a set of arguments
to an internal function, in lieu of closures in a Source implementation.

Hosting programs can expose VM-internal functions using the `sivmfn_vminternals`
array. Refer to the examples.
