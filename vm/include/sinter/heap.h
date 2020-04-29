#ifndef SINTER_HEAP_H
#define SINTER_HEAP_H

#include "config.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "opcode.h"
#include "fault.h"
#include "program.h"
#include "nanbox.h"
#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum __attribute__((__packed__)) {
  sitype_empty = 0,
  sitype_frame = 20,
  sitype_env = 21,
  sitype_strconst = 22,
  sitype_strpair = 23,
  sitype_string = 24,
  sitype_array = 25,
  sitype_array_data = 26,
  sitype_function = 27,
  sitype_free = 0xFF,
} siheap_type_t;
_Static_assert(sizeof(siheap_type_t) == 1, "siheap_type_t wider than needed");

#ifdef SINTER_STATIC_HEAP
extern unsigned char siheap[SINTER_HEAP_SIZE];
#else
extern unsigned char *siheap;
extern size_t siheap_size;
#endif

#ifdef SINTER_DEBUG
extern bool siheap_sweeping;
#endif

typedef address_t heapaddress_t;
#define SINTER_HEAPREF(addr) (siheap + addr)
#ifdef SINTER_DEBUG_MEMORY_CHECK
SINTER_INLINE bool siheap_inrange(const unsigned char *const ent) {
  return ent >= siheap && ent < siheap + SINTER_HEAP_SIZE;
}
#define SIHEAP_INRANGE(ent) (siheap_inrange((const unsigned char *) (ent)))
#else
#define SIHEAP_INRANGE(ent) (((unsigned char *) (ent)) < siheap + SINTER_HEAP_SIZE)
#endif
#define SIHEAP_PTRTONANBOX(ptr) NANBOX_OFPTR((uint32_t) (((unsigned char *) (ptr)) - siheap))
#define SIHEAP_NANBOXTOPTR(val) ((void *) (siheap + NANBOX_PTR(val)))

/**
 * The header of a heap allocation.
 */
typedef struct siheap_header {
  struct siheap_header *prev_node;
  /**
   * The size of the allocation, including the header.
   */
  address_t size;
  uint16_t refcount;
#ifdef SINTER_DEBUG_MEMORY_CHECK
  uint16_t debug_refcount;
#endif
  siheap_type_t type;
  _Bool flag_marked : 1;
  _Bool flag_destroying : 1;
  _Bool flag_displayed : 1;
  _Bool flag_internal_ref : 1;
} siheap_header_t;

typedef struct siheap_free {
  siheap_header_t header;
  struct siheap_free *prev_free;
  struct siheap_free *next_free;
} siheap_free_t;

extern siheap_free_t *siheap_first_free;

SINTER_INLINE void siheap_ref(void *vent) {
  assert(vent);
  siheap_header_t *ent = (siheap_header_t *) vent;
  assert(ent->type != sitype_free);
  ent->refcount += 1;
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
    assert(cur->prev_free != cur->next_free);
    assert(cur->prev_free->prev_free != cur->next_free || cur->next_free == NULL);
    assert(cur->prev_free->next_free == cur);
    cur->prev_free->next_free = cur->next_free;
  } else {
    assert(siheap_first_free == cur);
    siheap_first_free = cur->next_free;
  }
  if (cur->next_free) {
    assert(cur->next_free != cur->prev_free);
    assert(cur->next_free->next_free != cur->prev_free || cur->prev_free == NULL);
    assert(cur->next_free->prev_free == cur);
    cur->next_free->prev_free = cur->prev_free;
  }
}

SINTER_INLINE void siheap_free_fix_neighbours(siheap_free_t *cur) {
  if (cur->prev_free) {
    assert(cur->prev_free != cur);
    assert(cur->prev_free->prev_free != cur);
    cur->prev_free->next_free = cur;
  } else {
    assert(
      // inserting a new free node to the front of the list
      siheap_first_free == cur->next_free
      // from malloc_split: we've split siheap_first_free
      || cur->header.prev_node == &siheap_first_free->header
      // from mfree_inner: we're merging with the first free node, which is after us
      || (siheap_first_free > cur && &siheap_first_free->header < siheap_next(&cur->header)));
    siheap_first_free = cur;
  }
  if (cur->next_free) {
    assert(cur->next_free != cur);
    assert(cur->next_free->next_free != cur);
    cur->next_free->prev_free = cur;
  }
}

SINTER_INLINEIFC void siheap_init(void);
#ifndef __cplusplus
SINTER_INLINEIFC void siheap_init(void) {
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
}
#endif

void siheap_mark_sweep(void);

SINTER_INLINE siheap_free_t *siheap_malloc_find(address_t size) {
  bool sweeped = false;
  while (1) {
    siheap_free_t *cur = siheap_first_free;
    while (cur) {
      if (cur->header.size >= size) {
        break;
      }
      cur = cur->next_free;
    }

    if (!cur) {
      if (sweeped) {
        sifault(sinter_fault_out_of_memory);
        return NULL;
      } else {
        sweeped = true;
        siheap_mark_sweep();
        continue;
      }
    }

    return cur;
  }
}

SINTER_INLINEIFC siheap_header_t *siheap_malloc_split(siheap_free_t *cur, address_t size, siheap_type_t type);
#ifndef __cplusplus
SINTER_INLINEIFC siheap_header_t *siheap_malloc_split(siheap_free_t *cur, address_t size, siheap_type_t type) {
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
    assert(cur->prev_free != cur);
    assert(cur->next_free != cur);
    assert(cur->prev_free != cur->next_free || cur->prev_free == NULL);
    cur->header.size = size;
    siheap_free_fix_neighbours(newfree);
    siheap_fix_next(&newfree->header);
  } else {
    // no space for a free header
    siheap_free_remove(cur);
  }

  cur->header.type = type;
  cur->header.flag_destroying = cur->header.flag_displayed = cur->header.flag_marked = cur->header.flag_internal_ref = false;
  return &cur->header;
}
#endif

/**
 * Allocate memory.
 *
 * The allocation is returned with a reference count of 1.
 */
SINTER_INLINE siheap_header_t *siheap_malloc(address_t size, siheap_type_t type) {
  if (size < sizeof(siheap_free_t)) {
    size = sizeof(siheap_free_t);
  }

  siheap_free_t *free_block = siheap_malloc_find(size);
  siheap_header_t *allocated = siheap_malloc_split(free_block, size, type);
  siheap_ref(allocated);
  return allocated;
}

void siheap_mdestroy(siheap_header_t *ent);

SINTER_INLINE siheap_header_t *siheap_mfree_inner(siheap_header_t *ent) {
  assert(ent->size >= sizeof(siheap_free_t));
  if (ent->flag_marked) {
    SIBUGM("Freeing marked object\n");
    assert(false);
  }

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

    return prev;
  } else if (next_free) {
    // we have        [ent][free]
    // we'll merge to [free     ]

    siheap_free_t *const nextf = (siheap_free_t *) next;
    siheap_free_t *const entf = (siheap_free_t *) ent;

    assert(entf + 1 <= nextf);
    ent->size = ent->size + next->size;
    ent->type = sitype_free;
    ent->flag_destroying = ent->flag_displayed = ent->flag_marked = ent->flag_internal_ref = false;
    entf->prev_free = nextf->prev_free;
    entf->next_free = nextf->next_free;
    assert(entf->prev_free != entf);
    assert(entf->next_free != entf);
    assert(entf->prev_free != entf->next_free || entf->prev_free == NULL);

    siheap_free_fix_neighbours(entf);
    siheap_fix_next(ent);

    return ent;
  } else if (prev_free) {
    // we have        [free][ent]
    // we'll merge to [free     ]

    prev->size = prev->size + ent->size;
    siheap_fix_next(prev);

    return prev;
  } else {
    // create a new free entry
    siheap_free_t *const entf = (siheap_free_t *) ent;

    ent->type = sitype_free;
    ent->flag_destroying = ent->flag_displayed = ent->flag_marked = ent->flag_internal_ref = false;
    entf->prev_free = NULL;
    entf->next_free = siheap_first_free;
    assert(entf->next_free != entf);
    siheap_free_fix_neighbours(entf);

    return ent;
  }
}

SINTER_INLINE siheap_header_t *siheap_mfree(siheap_header_t *ent) {
  assert(ent->refcount == 0);
  assert(ent->type != sitype_free);
  siheap_mdestroy(ent);
  return siheap_mfree_inner(ent);
}

siheap_header_t *siheap_mrealloc(siheap_header_t *ent, address_t newsize);

SINTER_INLINE void siheap_deref(void *vent) {
  assert(vent);
  siheap_header_t *ent = (siheap_header_t *) vent;
  if (ent->flag_destroying) {
    // this object is in a cycle
    return;
  }
#ifdef SINTER_DEBUG
  assert(ent->refcount > 0 || siheap_sweeping);
  assert(ent->type != sitype_free || siheap_sweeping);
#endif

  if (ent->refcount) {
    ent->refcount -= 1;
    if (!ent->refcount && ent->type != sitype_free) {
      siheap_mfree(ent);
    }
  }
}

SINTER_INLINE void siheap_derefbox(sinanbox_t ent) {
  if (NANBOX_ISPTR(ent)) {
    siheap_deref(SIHEAP_NANBOXTOPTR(ent));
  }
}

#ifdef SINTER_DEBUG_MEMORY_CHECK
void debug_memorycheck(void);
void debug_memorycheck_search(const siheap_header_t *needle);
#endif

#ifdef __cplusplus
}
#endif

#endif
