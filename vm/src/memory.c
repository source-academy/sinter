#include "memory.h"

unsigned char sinter_heap[SINTER_HEAP_SIZE] = { 0 };

struct sinter_entry sinter_stack[SINTER_STACK_ENTRIES] = { 0 };

struct sinter_entry *sinter_stack_bottom = sinter_stack;
struct sinter_entry *sinter_stack_limit = sinter_stack;
struct sinter_entry *sinter_stack_top = sinter_stack;
