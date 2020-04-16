#ifndef SINTER_HEAP_H
#define SINTER_HEAP_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "config.h"
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
  sitype_strconst = 22,
  sitype_strpair = 23,
  sitype_string = 24,
  sitype_array = 25,
  sitype_array_data = 26,
  sitype_free = 0xFF
};

#ifdef SINTER_STATIC_HEAP
extern unsigned char siheap[SINTER_HEAP_SIZE];
#else
extern unsigned char *siheap;
extern size_t siheap_size;
#endif

typedef address_t heapaddress_t;
#define SINTER_HEAPREF(addr) (siheap + addr)
#define SIHEAP_INRANGE(ent) (((unsigned char *) (ent)) < siheap + SINTER_HEAP_SIZE)
#define SIHEAP_PTRTONANBOX(ptr) NANBOX_OFPTR((uint32_t) (((unsigned char *) (ptr)) - siheap))
#define SIHEAP_NANBOXTOPTR(val) ((void *) (siheap + NANBOX_PTR(val)))

/**
 * The header of a heap allocation.
 */
typedef struct siheap_header {
  uint16_t type;
  uint16_t refcount;
  struct siheap_header *prev_node;
  /**
   * The size of the allocation, including the header.
   */
  address_t size;
} siheap_header_t;

typedef struct siheap_free {
  siheap_header_t header;
  struct siheap_free *prev_free;
  struct siheap_free *next_free;
} siheap_free_t;

extern siheap_free_t *siheap_first_free;

SINTER_INLINE void siheap_ref(void *vent) {
  ++(((siheap_header_t *) vent)->refcount);
}

SINTER_INLINE void siheap_refbox(sinanbox_t ent) {
  if (NANBOX_ISPTR(ent)) {
    siheap_ref(SIHEAP_NANBOXTOPTR(ent));
  }
}

SINTER_INLINE siheap_header_t *siheap_next(siheap_header_t *const ent) {
  return (siheap_header_t *) (((unsigned char *) ent) + ent->size);
}

SINTER_INLINE void siheap_fix_next(siheap_header_t *const ent) {
  siheap_header_t *const next = siheap_next(ent);
  if (SIHEAP_INRANGE(next)) {
    next->prev_node = ent;
  }
}

SINTER_INLINE void siheap_free_remove(siheap_free_t *cur) {
  if (cur->prev_free) {
    cur->prev_free->next_free = cur->next_free;
  } else {
    assert(siheap_first_free == cur);
    siheap_first_free = cur->next_free;
  }
  if (cur->next_free) {
    cur->next_free->prev_free = cur->prev_free;
  }
}

SINTER_INLINE void siheap_free_fix_neighbours(siheap_free_t *cur) {
  if (cur->prev_free) {
    cur->prev_free->next_free = cur;
  } else {
    assert(siheap_first_free == cur->next_free || cur->header.prev_node == &siheap_first_free->header || !cur->next_free);
    siheap_first_free = cur;
  }
  if (cur->next_free) {
    cur->next_free->prev_free = cur;
  }
}

SINTER_INLINEIFC void siheap_init(void) SINTER_BODYIFC(
  siheap_first_free = (siheap_free_t *) siheap;
  *siheap_first_free = (siheap_free_t) {
    .header = {
      .type = sitype_free,
      .refcount = 0,
      .prev_node = NULL,
      .size = SINTER_HEAP_SIZE
    },
    .prev_free = NULL,
    .next_free = NULL
  };
)

SINTER_INLINEIFC siheap_free_t *siheap_malloc_find(address_t size) SINTER_BODYIFC(
  siheap_free_t *cur = siheap_first_free;
  while (cur) {
    if (cur->header.size >= size) {
      break;
    }
    cur = cur->next_free;
  }
  if (!cur) {
    sifault(sinter_fault_out_of_memory);
    return NULL;
  }

  return cur;
)

SINTER_INLINEIFC siheap_header_t *siheap_malloc_split(siheap_free_t *cur, address_t size, uint16_t type) SINTER_BODYIFC(
  if (size + sizeof(siheap_free_t) <= cur->header.size) {
    // enough space for a new free node
    // create one
    siheap_free_t *newfree = (siheap_free_t *) (((unsigned char *) cur) + size);
    *newfree = (siheap_free_t) {
      .header = {
        .type = sitype_free,
        .refcount = 0,
        .prev_node = &cur->header,
        .size = cur->header.size - size
      },
      .prev_free = cur->prev_free,
      .next_free = cur->next_free
    };
    cur->header.size = size;
    siheap_free_fix_neighbours(newfree);
    siheap_fix_next(&newfree->header);
  } else {
    // no space for a free header
    siheap_free_remove(cur);
  }
  siheap_ref(cur);
  cur->header.type = type;
  return &cur->header;
)

/**
 * Allocate memory.
 *
 * The allocation is returned with a reference count of 1.
 */
SINTER_INLINEIFC siheap_header_t *siheap_malloc(address_t size, uint16_t type) SINTER_BODYIFC(
  if (size < sizeof(siheap_free_t)) {
    size = sizeof(siheap_free_t);
  }

  siheap_free_t *free_block = siheap_malloc_find(size);
  return siheap_malloc_split(free_block, size, type);
)

void siheap_mdestroy(siheap_header_t *ent);

SINTER_INLINE void siheap_mfree_inner(siheap_header_t *ent) {
  assert(ent->size >= sizeof(siheap_free_t));

  siheap_header_t *const next = siheap_next(ent);
  siheap_header_t *const prev = ent->prev_node;
  const bool next_inrange = SIHEAP_INRANGE(next);
  const bool next_free = next_inrange && next->type == sitype_free;
  const bool prev_free = prev && prev->type == sitype_free;
  if (next_free && prev_free) {
    // we have        [free][ent][free]
    // we'll merge to [free           ]
    prev->size = prev->size + ent->size + next->size;
    siheap_fix_next(prev);
    siheap_free_remove((siheap_free_t *) next);
  } else if (next_free) {
    // we have        [ent][free]
    // we'll merge to [free     ]

    siheap_free_t *const nextf = (siheap_free_t *) next;
    siheap_free_t *const entf = (siheap_free_t *) ent;

    ent->size = ent->size + next->size;
    ent->type = sitype_free;
    entf->prev_free = nextf->prev_free;
    entf->next_free = nextf->next_free;

    siheap_free_fix_neighbours(entf);
    siheap_fix_next(ent);
  } else if (prev_free) {
    // we have        [free][ent]
    // we'll merge to [free     ]

    prev->size = prev->size + ent->size;
    siheap_fix_next(prev);
  } else {
    // create a new free entry
    siheap_free_t *const entf = (siheap_free_t *) ent;

    ent->type = sitype_free;
    entf->prev_free = NULL;
    entf->next_free = siheap_first_free;
    siheap_free_fix_neighbours(entf);
  }
}

SINTER_INLINE void siheap_mfree(siheap_header_t *ent) {
  assert(ent->refcount == 0);
  siheap_mfree_inner(ent);
  siheap_mdestroy(ent);
}

siheap_header_t *siheap_mrealloc(siheap_header_t *ent, address_t newsize);

SINTER_INLINE void siheap_deref(void *vent) {
  siheap_header_t *ent = (siheap_header_t *) vent;
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
