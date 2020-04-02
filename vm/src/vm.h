#ifndef SINTER_VM_H
#define SINTER_VM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "heap_obj.h"
#include "opcode.h"

struct sistate {
  bool running;
  enum sinter_fault fault_reason;
  const opcode_t *pc;
  const opcode_t *program;
  const opcode_t *program_end;
  struct siheap_env *env;
};

extern struct sistate sistate;

sinanbox_t siexec(const struct svm_function *fn);

#define SISTATE_CURADDR (sistate.pc - sistate.program)
#define SISTATE_ADDRTOPC(addr) (sistate.program + (addr))

#endif // SINTER_VM_H
