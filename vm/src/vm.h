#ifndef SINTER_VM_H
#define SINTER_VM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct sistate {
  bool running;
  size_t pc;
  const char *program;
};

#endif // SINTER_VM_H
