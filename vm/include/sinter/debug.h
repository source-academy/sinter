#ifndef SINTER_DEBUG_H
#define SINTER_DEBUG_H

#include "config.h"
#include "opcode.h"
#include "heap.h"

#if SINTER_DEBUG_LEVEL >= 1
#include <stdio.h>
#define SIDEBUG(...) fprintf(stderr, __VA_ARGS__)
#define SIDEBUG_NANBOX(v) debug_nanbox(v)

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
#define SIDEBUG(...) ((void) 0)
#define SIDEBUG_NANBOX(v) ((void) 0)

#define get_opcode_name(x) ""
#endif

#if SINTER_DEBUG_LEVEL >= 2
#include <stdio.h>
#define SITRACE(...) fprintf(stderr, __VA_ARGS__)
#else
#define SITRACE(...) ((void) 0)
#endif

#define SIBUG() SIDEBUG("BUG at %s at %s:%d\n", __func__, __FILE__, __LINE__);
#define SIBUGM(msg) SIDEBUG("BUG at %s at %s:%d: " msg, __func__, __FILE__, __LINE__);
#define SIBUGV(msg, ...) SIDEBUG("BUG at %s at %s:%d: " msg, __func__, __FILE__, __LINE__, __VA_ARGS__);
#endif
