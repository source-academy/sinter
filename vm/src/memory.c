#include "memory.h"

unsigned char siheap[SINTER_HEAP_SIZE] = { 0 };
struct siheap_free *siheap_first_free = (struct siheap_free *) siheap;

sinanbox_t sistack[SINTER_STACK_ENTRIES] = { 0 };

sinanbox_t *sistack_bottom = sistack;
sinanbox_t *sistack_limit = sistack;
sinanbox_t *sistack_top = sistack;
