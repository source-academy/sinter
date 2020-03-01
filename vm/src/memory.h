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
  sitype_empty = 0,
  sitype_stackframe = 20,
  sitype_environment = 21,
  sitype_free = 0xFF
};

// TODO: this may become a char * instead
// ESP32 has a limit on statically allocated memory, so in order to use the full
// memory, perhaps we will replace this with one large malloc() at the start
// but that adds an extra indirection for every memory access :(
extern unsigned char siheap[SINTER_HEAP_SIZE];

typedef address_t heapaddress_t;
#define SINTER_HEAPREF(addr) (siheap + addr)

#ifdef SINTER_USE_SINGLE
struct sientry {
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
// Future extension to use a double instead.
#error SINTER_USE_DOUBLE is currently unimplemented.
#endif

_Static_assert(sizeof(struct sientry) == 8, "struct sientry has wrong size");

struct siheap_header {
  uint16_t type;
  uint16_t refcount;
  struct siheap_header *prev_node;
  address_t size;
};

struct siheap_free {
  struct siheap_header header;
  struct siheap_free *prev_free;
  struct siheap_free *next_free;
};

struct siheap_environment {
  struct siheap_header header;
  uint16_t localcount;
  uint16_t argcount;
  struct sientry entry;
};

static inline struct sientry *sienv_getlocal(
  struct siheap_environment *env,
  uint16_t index) {
#ifndef SINTER_SEATBELTS_OFF
  if (index >= env->localcount) {
    sifault(sinter_fault_invalidld);
    return NULL;
  }
#endif

  return &env->entry + index;
}

static inline struct sientry *sienv_getarg(
  struct siheap_environment *env,
  uint16_t index) {
#ifndef SINTER_SEATBELTS_OFF
  if (index >= env->argcount) {
    sifault(sinter_fault_invalidld);
    return NULL;
  }
#endif

  return &env->entry + env->localcount + index;
}

struct siheap_function {
  struct siheap_header header;
  uint16_t maxstack;
  uint16_t localcount;
  opcode_t *code;
  struct siheap_environment *env;
};

struct siheap_stackframe {
  struct siheap_header header;
  struct sientry *saved_stack_bottom;
  struct sientry *saved_stack_limit;
  struct sientry *saved_stack_top;
  struct siheap_environment *saved_env;
};

extern struct sientry sistack[SINTER_STACK_ENTRIES];

// (Inclusive) Bottom of the current function's operand stack, as an index into
// sistack.
extern struct sientry *sistack_bottom;
// (Exclusive) Limit of the current function's operand stack, as an index into
// sistack.
extern struct sientry *sistack_limit;
// Index of the next empty entry of the current function's operand stack.
extern struct sientry *sistack_top;

static inline void stack_push(struct sientry entry) {
#ifndef SINTER_SEATBELTS_OFF
  if (sistack_top >= sistack_limit) {
    sifault(sinter_fault_stackoverflow);
    return;
  }
#endif

  *(sistack_top++) = entry;
}

static inline struct sientry sistack_pop(void) {
#ifndef SINTER_SEATBELTS_OFF
  if (sistack_top <= sistack_bottom) {
    sifault(sinter_fault_stackunderflow);
    return (struct sientry) { 0 };
  }
#endif

  return *(--sistack_top);
}

#endif
