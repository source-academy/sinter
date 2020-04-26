#include <sinter/vm.h>
#include <sinter/heap.h>
#include <sinter/stack.h>
#include <sinter/heap_obj.h>
#include <sinter/debug.h>
#include <sinter/debug_heap.h>
#include <sinter/config.h>

#ifdef SINTER_DEBUG_MEMORY_CHECK
/**
 * Do basic sanity checks on the heap, and reset the debug refcount.
 */
static void debug_memorycheck_walk_do_object_1(siheap_header_t *obj) {
  // either the next object is within the heap, or this object is the last object of the heap
  assert(SIHEAP_INRANGE(siheap_next(obj)) || ((unsigned char *) obj) + obj->size == siheap + SINTER_HEAP_SIZE);
  // this object doesn't start before the heap starts
  assert(((unsigned char *) obj) >= siheap);
  // this object ends before the heap ends
  assert(((unsigned char *) obj) + obj->size <= siheap + SINTER_HEAP_SIZE);
  // the next object's prev pointer is correct
  assert(!SIHEAP_INRANGE(siheap_next(obj)) || siheap_next(obj)->prev_node == obj);

  // reset the debug refcount
  obj->debug_refcount = 0;
}

static void debug_memorycheck_walk_check_nanboxes(sinanbox_t *arr, const size_t count, bool is_stack) {
  for (size_t i = 0; i < count; ++i) {
    sinanbox_t v = arr[i];
    if (!NANBOX_ISPTR(v)) {
      continue;
    }

    siheap_header_t *refobj = SIHEAP_NANBOXTOPTR(v);
    // check that this pointer is actually in range
    assert(SIHEAP_INRANGE(refobj));
    refobj->debug_refcount++;

    // check that the nanbox refers to something denotable
    switch (refobj->type) {
      case sitype_array_data:
      case sitype_free:
      case sitype_empty:
      case sitype_env:
      case sitype_marked:
      // string caches the result of flattening a strpair, it should never be
      // referred to in a nanbox
      case sitype_string:
      default:
        assert(false);
        break;
      case sitype_function:
      case sitype_array:
      case sitype_strconst:
      case sitype_strpair:
        break;
      case sitype_frame:
        // the stack has nanboxes referring to frames (fn return information)
        assert(is_stack);
        break;
    }
  }
}

/**
 * Do type-specific checks, and increment the refcount of all direct children.
 */
static void debug_memorycheck_walk_do_object_2(const siheap_header_t *obj) {
  switch (obj->type) {
  case sitype_array: {
    siheap_array_t *c = (siheap_array_t *) obj;

    assert(c->data->header.type == sitype_array_data);

    // check that the size of the allocated array data and the allocated count in the array object tally
    // note: >= because the heap allocator could over-allocate (in case it decides not to split the block)
    assert(c->data->header.size >= sizeof(siheap_array_data_t) + c->alloc_size*sizeof(sinanbox_t));

    // increase refcount of referent
    c->data->header.debug_refcount++;
    // increase refcount of data referents
    debug_memorycheck_walk_check_nanboxes(c->data->data, c->count, false);
    break;
  }

  case sitype_array_data: {
    // checking done above
    break;
  }

  case sitype_empty: {
    // invalid object type
    assert(false);
    break;
  }

  case sitype_env: {
    siheap_env_t *c = (siheap_env_t *) obj;

    // check that the entry count and allocated size tally
    // note: <= because the heap allocator could over-allocate (in case it decides not to split the block)
    assert(sizeof(siheap_env_t) + c->entry_count*sizeof(sinanbox_t) <= c->header.size);
    assert(!c->parent || SIHEAP_INRANGE(c->parent));

    if (c->parent) {
      c->parent->header.debug_refcount++;
    }
    // increase refcount of env referents
    debug_memorycheck_walk_check_nanboxes(c->entry, c->entry_count, false);
    break;
  }

  case sitype_frame: {
    siheap_frame_t *c = (siheap_frame_t *) obj;

    // check that the saved env is in the heap
    assert(!c->saved_env || SIHEAP_INRANGE(c->saved_env));

    // check that the saved stack top <= the saved stack limit
    assert(c->saved_stack_top <= c->saved_stack_limit);
    // check that the saved stack bottom <= the saved stack top
    assert(c->saved_stack_bottom <= c->saved_stack_top);

    // check that the saved stack bottom is in the stack
    assert(c->saved_stack_bottom >= sistack && c->saved_stack_bottom <= sistack + SINTER_STACK_ENTRIES);
    // check that the saved stack limit is in the stack
    assert(c->saved_stack_limit >= sistack && c->saved_stack_limit <= sistack + SINTER_STACK_ENTRIES);

    if (c->saved_env) {
      c->saved_env->header.debug_refcount++;
    }
    break;
  }

  case sitype_free: {
    siheap_free_t *c = (siheap_free_t *) obj;
    // check that the refcount is actually zero
    assert(c->header.refcount == 0);

    // check that the freelist pointers are correct
    assert(c->next_free == NULL || c->next_free->prev_free == c);
    assert((c->prev_free == NULL && siheap_first_free == c) || (c->prev_free && c->prev_free->next_free == c));
    break;
  }

  case sitype_function: {
    siheap_function_t *c = (siheap_function_t *) obj;

    // check that the code is in the program binary
    assert((const opcode_t *) c->code >= sistate.program && (const opcode_t *) c->code < sistate.program_end);

    // check that the environment is in the heap
    assert(SIHEAP_INRANGE(c->env));

    c->env->header.debug_refcount++;
    break;
  }

  case sitype_strconst: {
    siheap_strconst_t *c = (siheap_strconst_t *) obj;

    // check that the string is not NULL
    assert(c->string);

    break;
  }

  case sitype_string: {
    siheap_string_t *c = (siheap_string_t *) obj;

    // check that the allocation is actually big enough
    assert(c->size + sizeof(siheap_string_t) <= c->header.size);

    // check that the string is null-terminated
    assert(c->string[c->size - 1] == '\0');

    break;
  }

  case sitype_strpair: {
    siheap_strpair_t *c = (siheap_strpair_t *) obj;

    // check that the left pointer exists
    assert(SIHEAP_INRANGE(c->left));

    // check that the left points to the right thing
    assert(c->left->type == sitype_strpair || c->left->type == sitype_string || c->left->type == sitype_strconst);

    // check that the right pointer exists, or the string is flattened
    assert((!c->right && c->left->type == sitype_string) || SIHEAP_INRANGE(c->right));

    // check that the left points to the right type
    // the right CANNOT point to an sitype_string
    // that is only used to cache the result of flattening a strpair
    assert(!c->right || c->right->type == sitype_strpair || c->right->type == sitype_strconst);

    c->left->debug_refcount++;
    if (c->right) {
      c->right->debug_refcount++;
    }
    break;

  }

  case sitype_marked:
  default:
    assert(false);
    break;
  }

  if (obj->type != sitype_free) {
    assert(obj->refcount);
  }
}

static void debug_memorycheck_walk_do_object_3(const siheap_header_t *obj) {
  assert(obj->refcount == obj->debug_refcount);
}

#define WALK_HEAP(fn) do { \
  siheap_header_t *obj = (siheap_header_t *) siheap; \
  while (SIHEAP_INRANGE(obj)) { \
    (fn)(obj); \
    obj = siheap_next(obj); \
  } \
} while (0)

void debug_memorycheck(void) {
  WALK_HEAP(debug_memorycheck_walk_do_object_1);

  // walk the stack
  debug_memorycheck_walk_check_nanboxes(sistack, sistack_top - sistack, true);
  sistate.env->header.debug_refcount++;

  WALK_HEAP(debug_memorycheck_walk_do_object_2);
  WALK_HEAP(debug_memorycheck_walk_do_object_3);
}

static void debug_memorycheck_search_do_nanboxes(const sinanbox_t *arr, const size_t count, const siheap_header_t *needle, const siheap_header_t *container) {
  bool printed_header = false;
  for (size_t i = 0; i < count; ++i) {
    sinanbox_t v = arr[i];
    if (!NANBOX_ISPTR(v)) {
      continue;
    }

    siheap_header_t *refobj = SIHEAP_NANBOXTOPTR(v);

    if (refobj == needle) {
      if (!printed_header) {
        if (!container) {
          SIDEBUG("Stack:\n");
        } else {
          SIDEBUG("NaNbox children of ");
          SIDEBUG_HEAPOBJ(container);
          SIDEBUG(":\n");
        }
        printed_header = true;
      }
      SIDEBUG("  Entry at index %zu\n", i);
    }
  }
}

// LCOV_EXCL_START
// This part is meant for use from GDB; not part of the normal execution of the VM.
/**
 * Search the heap for referents
 */
static void debug_memorycheck_search_do_object(const siheap_header_t *needle, const siheap_header_t *obj) {
  switch (obj->type) {
  case sitype_array: {
    const siheap_array_t *c = (const siheap_array_t *) obj;
    if (&c->data->header == needle) {
      SIDEBUG("Array data of ");
      SIDEBUG_HEAPOBJ(obj);
      SIDEBUG("\n");
    }
    debug_memorycheck_search_do_nanboxes(c->data->data, c->count, needle, obj);
    break;
  }

  case sitype_env: {
    const siheap_env_t *c = (const siheap_env_t *) obj;

    if ((const siheap_header_t *) c->parent == needle) {
      SIDEBUG("Parent env. of ");
      SIDEBUG_HEAPOBJ(obj);
      SIDEBUG("\n");
    }

    debug_memorycheck_search_do_nanboxes(c->entry, c->entry_count, needle, obj);
    break;
  }

  case sitype_frame: {
    const siheap_frame_t *c = (const siheap_frame_t *) obj;

    if ((const siheap_header_t *) c->saved_env == needle) {
      SIDEBUG("Saved env. of ");
      SIDEBUG_HEAPOBJ(obj);
      SIDEBUG("\n");
    }

    break;
  }

  case sitype_function: {
    const siheap_function_t *c = (const siheap_function_t *) obj;

    if ((const siheap_header_t *) c->env == needle) {
      SIDEBUG("Env. of ");
      SIDEBUG_HEAPOBJ(obj);
      SIDEBUG("\n");
    }
    break;
  }

  case sitype_strpair: {
    const siheap_strpair_t *c = (const siheap_strpair_t *) obj;

    if ((const siheap_header_t *) c->left == needle) {
      SIDEBUG("Left string of ");
      SIDEBUG_HEAPOBJ(obj);
      SIDEBUG("\n");
    }

    if ((const siheap_header_t *) c->right == needle) {
      SIDEBUG("Left string of ");
      SIDEBUG_HEAPOBJ(obj);
      SIDEBUG("\n");
    }
    break;

  }

  case sitype_empty:
  case sitype_array_data:
  case sitype_free:
  case sitype_strconst:
  case sitype_string:
  case sitype_marked:
  default:
    break;
  }
}

void debug_memorycheck_search(const siheap_header_t *needle) {
  SIDEBUG("Searching for referents of ");
  SIDEBUG_HEAPOBJ(needle);
  SIDEBUG("\n");
  if (&sistate.env->header == needle) {
    SIDEBUG("Current environment\n");
  }
  debug_memorycheck_search_do_nanboxes(sistack, sistack_top - sistack, needle, NULL);

  siheap_header_t *obj = (siheap_header_t *) siheap;
  while (SIHEAP_INRANGE(obj)) {
    debug_memorycheck_search_do_object(needle, obj);
    obj = siheap_next(obj);
  }
}
// LCOV_EXCL_STOP
#endif
