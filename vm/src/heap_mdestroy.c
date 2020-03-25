#include "heap.h"
#include "heap_obj.h"

void siheap_mdestroy(struct siheap_header *ent) {
  switch (ent->type) {
  case sitype_env:
    sienv_destroy((struct siheap_env *) ent);
    break;
  case sinter_type_function:
    sifunction_destroy((struct siheap_function *) ent);
    break;
  }
}
