#include <sinter/config.h>
#include <sinter/heap.h>
#include <sinter/heap_obj.h>
#include <sinter/stack.h>

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
  case sinter_type_function:
    sifunction_destroy((siheap_function_t *) ent);
    break;
  }
}

void sistack_init(void) {
  sistack_bottom = sistack;
  sistack_limit = sistack;
  sistack_top = sistack;
}
