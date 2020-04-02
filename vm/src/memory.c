#include <sinter/heap.h>
#include <sinter/stack.h>

unsigned char siheap[SINTER_HEAP_SIZE] = { 0 };
siheap_free_t *siheap_first_free = (siheap_free_t *) siheap;

sinanbox_t sistack[SINTER_STACK_ENTRIES] = { 0 };

sinanbox_t *sistack_bottom = sistack;
sinanbox_t *sistack_limit = sistack;
sinanbox_t *sistack_top = sistack;
