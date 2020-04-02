#ifndef SINTER_DEBUG_HEAP_H
#define SINTER_DEBUG_HEAP_H

#include "inline.h"
#include "debug.h"
#include "heap.h"
#include "vm.h"

#if SINTER_DEBUG_LEVEL >= 1
#define SIDEBUG_NANBOX(v) debug_nanbox(v)

void debug_heap_obj(siheap_header_t *o);
void debug_nanbox(sinanbox_t v);
#else
#define SIDEBUG_NANBOX(v) ((void) 0)
#endif

#endif
