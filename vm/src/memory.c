#include <sinter/heap.h>
#include <sinter/heap_obj.h>
#include <sinter/stack.h>

typedef struct siheap_free {
  siheap_header_t header;
  struct siheap_free *prev_free;
  struct siheap_free *next_free;
} siheap_free_t;

#ifdef SINTER_STATIC_HEAP
unsigned char siheap[SINTER_HEAP_SIZE] = { 0 };
#else
unsigned char *siheap = NULL;
size_t siheap_size = 0;
#endif

siheap_free_t *siheap_first_free = NULL;

sinanbox_t sistack[SINTER_STACK_ENTRIES] = { 0 };

sinanbox_t *sistack_bottom = sistack;
sinanbox_t *sistack_limit = sistack;
sinanbox_t *sistack_top = sistack;

static inline siheap_header_t *siheap_next(siheap_header_t *const ent) {
  return (siheap_header_t *) (((unsigned char *) ent) + ent->size);
}

static inline void siheap_fix_next(siheap_header_t *const ent) {
  siheap_header_t *const next = siheap_next(ent);
  if (SIHEAP_INRANGE(next)) {
    next->prev_node = ent;
  }
}

static inline void siheap_free_remove(siheap_free_t *cur) {
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

static inline void siheap_free_fix_neighbours(siheap_free_t *cur) {
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

void siheap_init(void) {
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

/**
 * Allocate memory.
 *
 * The allocation is returned with a reference count of 1.
 */
siheap_header_t *siheap_malloc(address_t size, uint16_t type) {
  if (size < sizeof(siheap_free_t)) {
    size = sizeof(siheap_free_t);
  }

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
}

static inline void sifunction_destroy(siheap_function_t *fn) {
  if (fn->env) {
    siheap_deref(fn->env);
  }
}

/**
 * Runs the destructor for the given heap object.
 *
 * The destructor typically just decrements the reference counts
 * of objects referred to by the given object.
 */
void siheap_mdestroy(siheap_header_t *ent) {
  switch (ent->type) {
  case sitype_env:
    sienv_destroy((siheap_env_t *) ent);
    break;
  case sinter_type_function:
    sifunction_destroy((siheap_function_t *) ent);
    break;
  }
}

void siheap_mfree(siheap_header_t *ent) {
  assert(ent->size >= sizeof(siheap_free_t));
  assert(ent->refcount == 0);

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

  siheap_mdestroy(ent);
}

void sistack_init(void) {
  sistack_bottom = sistack;
  sistack_limit = sistack;
  sistack_top = sistack;
}
