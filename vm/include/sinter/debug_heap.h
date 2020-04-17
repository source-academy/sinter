#ifndef SINTER_DEBUG_HEAP_H
#define SINTER_DEBUG_HEAP_H

#include "config.h"
#include "opcode.h"
#include "debug.h"
#include "heap.h"

#if SINTER_DEBUG_LEVEL >= 1
#define SIDEBUG_NANBOX(v) debug_nanbox(v)
#define SIDEBUG_HEAPOBJ(v) debug_heap_obj(v)

#ifdef __cplusplus
extern "C" {
#endif

const char *get_opcode_name(opcode_t op);
void debug_heap_obj(siheap_header_t *o);
void debug_nanbox(sinanbox_t v);

#ifdef __cplusplus
}
#endif
#else
#define SIDEBUG_NANBOX(v) ((void) 0)
#define SIDEBUG_HEAPOBJ(v) ((void) 0)
#define get_opcode_name(x) ""
#endif

#endif
