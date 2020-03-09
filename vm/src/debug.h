#ifndef SINTER_DEBUG_H
#define SINTER_DEBUG_H

#include "nanbox.h"

#ifdef SINTER_DEBUG_LOG
#include <stdio.h>
#define SIDEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define SIDEBUG(...) (void) 0
#endif

#define SIBUG() SIDEBUG("BUG at %s at %s:%d\n", __func__, __FILE__, __LINE__);
#define SIBUGM(msg) SIDEBUG("BUG at %s at %s:%d: " msg, __func__, __FILE__, __LINE__);
#define SIBUGV(msg, ...) SIDEBUG("BUG at %s at %s:%d: " msg, __func__, __FILE__, __LINE__, __VA_ARGS__);

#endif
