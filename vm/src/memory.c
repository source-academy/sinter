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

SINTER_INLINE void siheap_markbox(sinanbox_t ent);
SINTER_INLINE void siheap_mark(siheap_header_t *ent);

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


SINTER_INLINE void siheap_markbox(sinanbox_t ent) {
  if (NANBOX_ISPTR(ent)) {
    siheap_mark(SIHEAP_NANBOXTOPTR(ent));
  }
}

SINTER_INLINE void siheap_mark(siheap_header_t *vent) {
  if (!(vent->type & 0x8000)) {
    vent->type |= 0x8000;
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
      siheap_mark(&(env)->parent->header);
      break;
    }
    }
   }
 }

SINTER_INLINE void siheap_sweep() {
  //cast heap into proper header
  siheap_header_t *curr = (siheap_header_t *) siheap;
  while(SIHEAP_INRANGE(curr)) {
    if(!(curr -> type &= 0x8000)) {
      curr->refcount = 0;
      siheap_mfree(curr);
      curr =  siheap_next(curr);
    }
    curr-> type &= 0x7FFF;//unmark nodes
  }
}


SINTER_INLINE void siheap_mark_sweep() {
   sinanbox_t *curr = sistack_top-1;
   while(curr!=sistack_bottom){
      siheap_markbox(*curr);
      curr--;
   }
   siheap_mark(&(sistate.env)->header);
   siheap_sweep();
}
void sistack_init(void) {
  sistack_bottom = sistack;
  sistack_limit = sistack;
  sistack_top = sistack;
}
