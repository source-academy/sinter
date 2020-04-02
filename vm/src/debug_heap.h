#ifndef SINTER_DEBUG_HEAP_H
#define SINTER_DEBUG_HEAP_H

#include "debug.h"
#include "heap.h"
#include "vm.h"

#if SINTER_DEBUG_LEVEL >= 1
#include <stdio.h>
#include <inttypes.h>

#define SIDEBUG_NANBOX(v) debug_nanbox(v)

static inline void debug_heap_obj(siheap_header_t *o) {
  switch (o->type) {
  case sitype_env: {
    siheap_env_t *env = (siheap_env_t *) o;
    SIDEBUG("environment with %d entries; parent at %p", env->entry_count, (void *) env->parent);
    break;
  }
  case sitype_frame: {
    siheap_frame_t *fr = (siheap_frame_t *) o;
    SIDEBUG("frame; return address %tx, stack bottom %p, limit %p, top %p, saved environment %p", fr->return_address - sistate.program,
      (void *) fr->saved_stack_bottom, (void *) fr->saved_stack_limit, (void *) fr->saved_stack_top, (void *) fr->saved_env);
    break;
  }
  case sinter_type_function: {
    siheap_function_t *f = (siheap_function_t *) o;
    SIDEBUG("function; code address %tx, environment %p",
      (const opcode_t *) f->code - sistate.program, (void *) f->env);
    break;
  }
  }
}

static inline void debug_nanbox(sinanbox_t v) {
  switch (NANBOX_GETTYPE(v)) {
  NANBOX_CASES_TINT
    SIDEBUG(("integer, value: %" PRId32), NANBOX_INT(v));
    break;
  case NANBOX_TBOOL:
    SIDEBUG("boolean, value: %d", NANBOX_BOOL(v));
    break;
  case NANBOX_TUNDEF:
    SIDEBUG("undefined");
    break;
  case NANBOX_TNULL:
    SIDEBUG("null");
    break;
  case NANBOX_TIFN:
    SIDEBUG("internal function, type: %d, number: %d", NANBOX_IFN_TYPE(v), NANBOX_IFN_NUMBER(v));
    break;
  NANBOX_CASES_TPTR
    SIDEBUG("pointer to ");
    debug_heap_obj(SIHEAP_NANBOXTOPTR(v));
    break;
  default:
    if (NANBOX_ISFLOAT(v)) {
      SIDEBUG("float, value: %f", NANBOX_FLOAT(v));
    } else {
      SIDEBUG("unknown NaNbox value: %08x", v.as_i32);
    }
    break;
  }
}
#else
#define SIDEBUG_NANBOX(v) ((void) 0)
#endif

#endif
