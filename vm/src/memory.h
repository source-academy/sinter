#ifndef SINTER_MEMORY_H
#define SINTER_MEMORY_H

#include <stdbool.h>
#include <stddef.h>

#include "opcode.h"
#include "fault.h"

#ifndef SINTER_HEAP_SIZE
// "64 KB ought to be enough for anybody"
#define SINTER_HEAP_SIZE 0x10000
#endif

#ifndef SINTER_STACK_ENTRIES
#define SINTER_STACK_ENTRIES 0x200
#endif

#if !defined(SINTER_USE_SINGLE) && !defined(SINTER_USE_DOUBLE)
#define SINTER_USE_SINGLE
#endif

#if defined(SINTER_USE_SINGLE) && defined(SINTER_USE_DOUBLE)
#error Cannot have both SINTER_USE_SINGLE and SINTER_USE_DOUBLE defined.
#endif

enum {
  sinter_type_empty = 0,
  sinter_type_undefined = 1,
  sinter_type_null = 2,
  sinter_type_boolean = 3,
  sinter_type_integer = 4,
  sinter_type_float = 5,
  sinter_type_string = 6,
  sinter_type_array = 7,
  sinter_type_function = 8,
  sinter_type_stackframe = 9,
  sinter_type_environment = 10,
  sinter_type_free = 0xFF
};

// TODO: this may become a char * instead
// ESP32 has a limit on statically allocated memory, so in order to use the full
// memory, perhaps we will replace this with one large malloc() at the start
// but that adds an extra indirection for every memory access :(
extern unsigned char sinter_heap[SINTER_HEAP_SIZE];

// If pointer size fits in 4 bytes, we can store direct pointers in our stack
// entries, to save one level of indirection.
#if __SIZEOF_POINTER__ <= 4
typedef unsigned char *heapaddress_t;
#define SINTER_HEAPREF(addr) (addr)
#else
typedef address_t heapaddress_t;
#define SINTER_HEAPREF(addr) (sinter_heap + addr)
#endif

#ifdef SINTER_USE_SINGLE
struct sinter_entry {
  uint32_t type;
  union {
    // Used for pointers into the heap, for strings, arrays, functions, and
    // stackframes
    heapaddress_t pointer_value;
    // Used for actual integers, and booleans
    int32_t integer_value;
    // Used for floats.
    float float_value;
  };
};

#define SINTER_ENTRY_TYPE(entry) ((entry).type)
#define SINTER_ENTRY_PTRVAL(entry) ((entry).pointer_value)
#define SINTER_ENTRY_HEAPPTR(entry) (SINTER_HEAPREF((entry).pointer_value))
#define SINTER_ENTRY_INT(entry) ((entry).integer_value)
#define SINTER_ENTRY_FLOAT(entry) ((entry).float_value)
#endif

#ifdef SINTER_USE_DOUBLE
// Future extension to use a double instead
// Then we will probably use NaN-boxing, Webkit JSC-style.
#error SINTER_USE_DOUBLE is currently unimplemented.

#define SINTER_ENTRY_TYPE(entry) (TODO)
#define SINTER_ENTRY_PTRVAL(entry) (TODO)
#define SINTER_ENTRY_HEAPPTR(entry) (TODO)
#define SINTER_ENTRY_INT(entry) (TODO)
#define SINTER_ENTRY_FLOAT(entry) (TODO)
#endif

_Static_assert(sizeof(struct sinter_entry) == 8, "struct sinter_entry has wrong size");

struct sinter_heap_header {
  uint16_t type;
  uint16_t refcount;
  struct sinter_heap_header *prev_node;
  address_t size;
};

struct sinter_heap_free {
  struct sinter_heap_header header;
  struct sinter_heap_free *prev_free;
  struct sinter_heap_free *next_free;
};

struct sinter_heap_environment {
  struct sinter_heap_header header;
  uint16_t localcount;
  uint16_t argcount;
  struct sinter_entry entry;
};

static inline struct sinter_entry *sinter_env_getlocal(
  struct sinter_heap_environment *env,
  uint16_t index) {
#ifndef SINTER_SEATBELTS_OFF
  if (index >= env->localcount) {
    sinter_fault(sinter_fault_invalidld);
    return NULL;
  }
#endif

  return &env->entry + index;
}

static inline struct sinter_entry *sinter_env_getarg(
  struct sinter_heap_environment *env,
  uint16_t index) {
#ifndef SINTER_SEATBELTS_OFF
  if (index >= env->argcount) {
    sinter_fault(sinter_fault_invalidld);
    return NULL;
  }
#endif

  return &env->entry + env->localcount + index;
}

struct sinter_heap_function {
  struct sinter_heap_header header;
  uint16_t maxstack;
  uint16_t localcount;
  opcode_t *code;
  struct sinter_heap_environment *env;
};

struct sinter_heap_stackframe {
  struct sinter_heap_header header;
  struct sinter_entry *saved_stack_bottom;
  struct sinter_entry *saved_stack_limit;
  struct sinter_entry *saved_stack_top;
  struct sinter_heap_environment *saved_env;
};

extern struct sinter_entry sinter_stack[SINTER_STACK_ENTRIES];

// (Inclusive) Bottom of the current function's operand stack, as an index into
// sinter_stack.
extern struct sinter_entry *sinter_stack_bottom;
// (Exclusive) Limit of the current function's operand stack, as an index into
// sinter_stack.
extern struct sinter_entry *sinter_stack_limit;
// Index of the next empty entry of the current function's operand stack.
extern struct sinter_entry *sinter_stack_top;

static inline void sinter_stack_push(struct sinter_entry entry) {
#ifndef SINTER_SEATBELTS_OFF
  if (sinter_stack_top >= sinter_stack_limit) {
    sinter_fault(sinter_fault_stackoverflow);
    return;
  }
#endif

  *(sinter_stack_top++) = entry;
}

static inline struct sinter_entry sinter_stack_pop(void) {
#ifndef SINTER_SEATBELTS_OFF
  if (sinter_stack_top <= sinter_stack_bottom) {
    sinter_fault(sinter_fault_stackunderflow);
    return (struct sinter_entry) { 0 };
  }
#endif

  return *(--sinter_stack_top);
}

#endif
