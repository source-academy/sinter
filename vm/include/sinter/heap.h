#ifndef SINTER_HEAP_H
#define SINTER_HEAP_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "inline.h"
#include "opcode.h"
#include "fault.h"
#include "program.h"
#include "nanbox.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  sitype_empty = 0,
  sitype_frame = 20,
  sitype_env = 21,
  sitype_free = 0xFF
};

#ifdef SINTER_STATIC_HEAP
#ifndef SINTER_HEAP_SIZE
// "64 KB ought to be enough for anybody"
#define SINTER_HEAP_SIZE 0x10000
#endif
extern unsigned char siheap[SINTER_HEAP_SIZE];
#else
#include "heap_extern.h"
#define SINTER_HEAP_SIZE (siheap_size)
#endif

typedef address_t heapaddress_t;
#define SINTER_HEAPREF(addr) (siheap + addr)
#define SIHEAP_INRANGE(ent) (((unsigned char *) (ent)) < siheap + SINTER_HEAP_SIZE)
#define SIHEAP_PTRTONANBOX(ptr) NANBOX_OFPTR((uint32_t) (((unsigned char *) (ptr)) - siheap))
#define SIHEAP_NANBOXTOPTR(val) ((void *) (siheap + NANBOX_PTR(val)))

typedef struct siheap_header {
  uint16_t type;
  uint16_t refcount;
  struct siheap_header *prev_node;
  address_t size;
} siheap_header_t;

void siheap_init(void);
siheap_header_t *siheap_malloc(address_t size, uint16_t type);
void siheap_mdestroy(siheap_header_t *ent);
void siheap_mfree(siheap_header_t *ent);

SINTER_INLINE void siheap_ref(void *vent) {
  ++(((siheap_header_t *) vent)->refcount);
}

SINTER_INLINE void siheap_refbox(sinanbox_t ent) {
  if (NANBOX_ISPTR(ent)) {
    siheap_ref(SIHEAP_NANBOXTOPTR(ent));
  }
}

SINTER_INLINE void siheap_deref(void *vent) {
  siheap_header_t *ent = vent;
  if (--(ent->refcount)) {
    return;
  }

  siheap_mfree(ent);
}

SINTER_INLINE void siheap_derefbox(sinanbox_t ent) {
  if (NANBOX_ISPTR(ent)) {
    siheap_deref(SIHEAP_NANBOXTOPTR(ent));
  }
}

#ifdef __cplusplus
}
#endif

#endif
