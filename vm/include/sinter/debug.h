#ifndef SINTER_DEBUG_H
#define SINTER_DEBUG_H

#include "opcode.h"

#if SINTER_DEBUG_LEVEL >= 1
#include <stdio.h>
#define SIDEBUG(...) fprintf(stderr, __VA_ARGS__)
const char *get_opcode_name(opcode_t op);
#else
#define SIDEBUG(...) ((void) 0)
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
