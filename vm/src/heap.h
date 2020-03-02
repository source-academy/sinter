#ifndef SINTER_HEAP_H
#define SINTER_HEAP_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "opcode.h"
#include "fault.h"
#include "program.h"
#include "nanbox.h"

#ifndef SINTER_HEAP_SIZE
// "64 KB ought to be enough for anybody"
#define SINTER_HEAP_SIZE 0x10000
#endif

#ifndef SINTER_STACK_ENTRIES
#define SINTER_STACK_ENTRIES 0x200
#endif

enum {
  sitype_empty = 0,
  sitype_frame = 20,
  sitype_env = 21,
  sitype_free = 0xFF
};

// TODO: this may become a char * instead
// ESP32 has a limit on statically allocated memory, so in order to use the full
// memory, perhaps we will replace this with one large malloc() at the start
// but that adds an extra indirection for every memory access :(
extern unsigned char siheap[SINTER_HEAP_SIZE];

typedef address_t heapaddress_t;
#define SINTER_HEAPREF(addr) (siheap + addr)
#define SIHEAP_INRANGE(ent) (((unsigned char *) (ent)) < siheap + SINTER_HEAP_SIZE)
#define SIHEAP_PTRTONANBOX(ptr) NANBOX_OFPTR((uint32_t) (((unsigned char *) (ptr)) - siheap))
#define SIHEAP_NANBOXTOPTR(val) ((void *) (siheap + NANBOX_PTR(val)))

struct siheap_header {
  uint16_t type;
  uint16_t refcount;
  struct siheap_header *prev_node;
  address_t size;
};

static inline struct siheap_header *siheap_next(struct siheap_header *const ent) {
  return (struct siheap_header *) (((unsigned char *) ent) + ent->size);
}

static inline void siheap_fix_next(struct siheap_header *const ent) {
  struct siheap_header *const next = siheap_next(ent);
  if (SIHEAP_INRANGE(next)) {
    next->prev_node = ent;
  }
}

struct siheap_free {
  struct siheap_header header;
  struct siheap_free *prev_free;
  struct siheap_free *next_free;
};

extern struct siheap_free *siheap_first_free;

static inline void siheap_free_remove(struct siheap_free *cur) {
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

static inline void siheap_free_fix_neighbours(struct siheap_free *cur) {
  if (cur->prev_free) {
    cur->prev_free->next_free = cur;
  } else {
    assert(siheap_first_free == cur->next_free);
    siheap_first_free = cur;
  }
  if (cur->next_free) {
    cur->next_free->prev_free = cur;
  }
}

static inline void siheap_init(void) {
  siheap_first_free = (struct siheap_free *) siheap;
  *siheap_first_free = (struct siheap_free) {
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

static inline void siheap_ref(void *vent) {
  ++(((struct siheap_header *) vent)->refcount);
}

static inline struct siheap_header *siheap_malloc(address_t size, uint16_t type) {
  if (size < sizeof(struct siheap_free)) {
    size = sizeof(struct siheap_free);
  }

  struct siheap_free *cur = siheap_first_free;
  while (cur) {
    if (cur->header.size >= size) {
      break;
    }
    cur = cur->next_free;
  }
  if (!cur) {
    return NULL;
  }

  if (size + sizeof(struct siheap_free) <= cur->header.size) {
    // enough space for a new free node
    // create one
    struct siheap_free *newfree = (struct siheap_free *) (((unsigned char *) cur) + size);
    *newfree = (struct siheap_free) {
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
}

static inline void siheap_mfree(struct siheap_header *ent) {
  assert(ent->size >= sizeof(struct siheap_free));
  assert(ent->refcount == 0);

  struct siheap_header *const next = siheap_next(ent);
  struct siheap_header *const prev = ent->prev_node;
  const bool next_inrange = SIHEAP_INRANGE(next);
  const bool next_free = next_inrange && next->type == sitype_free;
  const bool prev_free = prev && prev->type == sitype_free;
  if (next_free && prev_free) {
    // we have        [free][ent][free]
    // we'll merge to [free           ]
    prev->size = prev->size + ent->size + next->size;
    siheap_fix_next(prev);
    siheap_free_remove((struct siheap_free *) next);
  } else if (next_free) {
    // we have        [ent][free]
    // we'll merge to [free     ]

    struct siheap_free *const nextf = (struct siheap_free *) next;
    struct siheap_free *const entf = (struct siheap_free *) ent;

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
    struct siheap_free *const entf = (struct siheap_free *) ent;

    ent->type = sitype_free;
    entf->prev_free = NULL;
    entf->next_free = siheap_first_free;
    siheap_free_fix_neighbours(entf);
  }

  // TODO decrease refcount of referents
}

static inline void siheap_deref(void *vent) {
  struct siheap_header *ent = vent;
  if (--(ent->refcount)) {
    return;
  }

  siheap_mfree(ent);
}

#endif
