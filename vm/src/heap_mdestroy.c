#include <sinter/heap.h>
#include <sinter/heap_obj.h>

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
