#include <string.h>

#include <sinter/config.h>
#include <sinter/heap.h>
#include <sinter/heap_obj.h>
#include <sinter/stack.h>
#include <sinter/vm.h>
#include <sinter/nanbox.h>

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
  case sitype_strpair:
    sistrpair_destroy((siheap_strpair_t *) ent);
    break;
  case sitype_array:
    siarray_destroy((siheap_array_t *) ent);
    break;
  case sitype_function:
    sifunction_destroy((siheap_function_t *) ent);
    break;
  case sitype_array_data:
  case sitype_frame:
  case sitype_strconst:
  case sitype_string:
    break;
  default:
  case sitype_marked:
  case sitype_empty:
  case sitype_free:
    SIBUGV("Attempting to destroy object of type %d\n", ent->type);
    assert(false);
    break;
  }
}

static void siheap_mark(siheap_header_t *vent);

static inline void siheap_markbox(sinanbox_t ent) {
  if (NANBOX_ISPTR(ent)) {
    siheap_mark(SIHEAP_NANBOXTOPTR(ent));
  }
}

static void siheap_mark(siheap_header_t *vent) {
  if (!SIHEAP_INRANGE(vent) || ((unsigned char *) vent) < siheap) {
    return;
  }

  if (!(vent->type & 0x8000)) {
#if SINTER_DEBUG_LOGLEVEL >= 2
    SIDEBUG("Marking object ");
    SIDEBUG_HEAPOBJ(vent);
    SIDEBUG("\n");
#endif

    // object is unmarked; mark it
    vent->type |= 0x8000;

    // mark any children
    switch (vent->type & 0x7FFF) {
    case sinter_type_function:
      siheap_mark(&((siheap_function_t *) vent)->env->header);
      break;
    case sitype_frame:
      siheap_mark(&((siheap_frame_t *) vent)->saved_env->header);
      break;
    case sitype_env: {
      siheap_env_t *env = (siheap_env_t *) vent;
      for (size_t i = 0; i < env->entry_count; i++) {
        siheap_markbox(env->entry[i]);
      }
      siheap_mark(&env->parent->header);
      break;
    }
    case sitype_array: {
      siheap_array_t *a = (siheap_array_t *) vent;
      for (address_t i = 0; i < a->count; ++i) {
        siheap_markbox(a->data->data[i]);
      }
      break;
    }
    case sitype_strconst:
    case sitype_string:
      // These types have no children, no need to do anything
      break;
    default:
      SIBUGV("Unknown type %d", vent->type & 0x7FFF);
      break;
    }
  }
}

static inline void siheap_sweep(void) {
  siheap_header_t *curr = (siheap_header_t *) siheap;
  while (SIHEAP_INRANGE(curr)) {
    if (!(curr->type & 0x8000)) {
      curr->refcount = 0;
#if SINTER_DEBUG_LOGLEVEL >= 2
      SIDEBUG("Sweeping object ");
      SIDEBUG_HEAPOBJ(curr);
      SIDEBUG("\n");
#endif
      siheap_mfree(curr);
    } else {
      curr->type &= 0x7FFF;
    }
    curr = siheap_next(curr);
  }
}

void siheap_mark_sweep(void) {
   sinanbox_t *curr = sistack_top - 1;
   while (curr >= sistack) {
      siheap_markbox(*(curr--));
   }
   siheap_mark(&sistate.env->header);
   siheap_sweep();
}

void sistack_init(void) {
  sistack_bottom = sistack;
  sistack_limit = sistack;
  sistack_top = sistack;
}

static address_t sizeof_strobj(siheap_header_t *obj) {
  switch (obj->type) {
  case sitype_strconst: {
    siheap_strconst_t *v = (siheap_strconst_t *) obj;
    return v->string->length - 1;
  }

  case sitype_strpair: {
    siheap_strpair_t *v = (siheap_strpair_t *) obj;
    return sizeof_strobj(v->left) + sizeof_strobj(v->right);
  }

  case sitype_string: {
    siheap_string_t *v = (siheap_string_t *) obj;
    return v->header.size - sizeof(siheap_string_t) - 1;
  }

  case sitype_array_data:
  case sitype_empty:
  case sitype_frame:
  case sitype_free:
  case sitype_marked:
  case sitype_env:
  case sitype_array:
  case sitype_function:
  default:
    SIBUGM("Unknown string type\n");
    sifault(sinter_fault_internal_error);
    break;
  }
}

static void write_strobj(siheap_header_t *obj, char **to) {
  if (!obj) {
    return;
  }
  switch (obj->type) {
  case sitype_strconst: {
    siheap_strconst_t *v = (siheap_strconst_t *) obj;
    const address_t size = v->string->length - 1;
    memcpy(*to, v->string->data, size);
    *to += size;
    return;
  }

  case sitype_strpair: {
    siheap_strpair_t *v = (siheap_strpair_t *) obj;
    write_strobj(v->left, to);
    write_strobj(v->right, to);
    return;
  }

  case sitype_string: {
    siheap_string_t *v = (siheap_string_t *) obj;
    const address_t size = v->size - 1;
    memcpy(*to, v->string, size);
    *to += size;
    return;
  }

  case sitype_array_data:
  case sitype_empty:
  case sitype_frame:
  case sitype_free:
  case sitype_marked:
  case sitype_env:
  case sitype_array:
  case sitype_function:
  default:
    SIBUGM("Unknown string type\n");
    sifault(sinter_fault_internal_error);
    break;
  }
}

siheap_string_t *sistrpair_flatten(siheap_strpair_t *obj) {
  if (!obj->right) {
    return (siheap_string_t *) obj->left;
  }

  address_t strsize = sizeof_strobj(&obj->header);
  siheap_string_t *string = sistring_new(strsize + 1);

  char *to = string->string;
  write_strobj(&obj->header, &to);
  *to = '\0';

  siheap_deref(obj->left);
  siheap_deref(obj->right);

  obj->left = &string->header;
  obj->right = NULL;
  return string;
}

siheap_header_t *siheap_mrealloc(siheap_header_t *ent, address_t newsize) {
  if (ent->size >= newsize) {
    // we don't support shrinking currently
    return ent;
  }

  siheap_header_t *next = siheap_next(ent);
  if (SIHEAP_INRANGE(next) && next->type == sitype_free && ent->size + next->size >= newsize) {
    // the next block is free and large enough

    // do the allocation on the block
    siheap_malloc_split((siheap_free_t *) next, newsize - ent->size, ent->type);

    // now merge our two heap blocks
    ent->size += next->size;
    siheap_fix_next(ent);

    return ent;
  }

  // the next block is not free and/or not large enough

  // store the type, refcount and size of the current block
  siheap_type_t orig_type = ent->type;
  uint16_t orig_refcount = ent->refcount;
  address_t orig_size = ent->size;

  // if the previous node is a free block, then we free BEFORE malloc
  // so there is a chance that the new merged free block is large enough
  // we cannot do this all the time as in some cases (if a new free node is
  // constructed in our current memory block) our array data will be overwritten
  const bool free_first = ((unsigned char *) ent->prev_node) >= siheap
    && ent->prev_node->type == sitype_free;
  ent->refcount = 0;

  if (free_first) {
    siheap_mfree_inner(ent);
  }

  // now allocate a new block with the new size and original type
  siheap_header_t *new_alloc = siheap_malloc(newsize, orig_type);

  if (!free_first) {
    siheap_mfree_inner(ent);
  }
  // restore the refcount
  new_alloc->refcount = orig_refcount;

  // move the contents over from the old block
  memmove(new_alloc + 1, ent + 1, orig_size - sizeof(siheap_header_t));

  return new_alloc;
}
