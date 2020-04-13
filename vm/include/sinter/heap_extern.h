#ifndef SINTER_HEAP_EXTERN_H
#define SINTER_HEAP_EXTERN_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SINTER_STATIC_HEAP
extern unsigned char *siheap;
extern size_t siheap_size;
#endif

#ifdef __cplusplus
}
#endif

#endif
