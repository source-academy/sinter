#ifndef SINTER_VM_H
#define SINTER_VM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct sistate {
  bool running;
  const opcode_t *pc;
  const opcode_t *program;
};

#endif // SINTER_VM_H
