#ifndef SINTER_VM_H
#define SINTER_VM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "inline.h"
#include "nanbox.h"
#include "heap_obj.h"
#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sistate {
  bool running;
  sinter_fault_t fault_reason;
  const opcode_t *pc;
  const opcode_t *program;
  const opcode_t *program_end;
  siheap_env_t *env;
};

extern struct sistate sistate;

/**
 * The type of a VM-internal function.
 *
 * A VM-internal function should not modify the stack.
 */
typedef sinanbox_t (*sivmfnptr_t)(uint8_t argc, sinanbox_t *argv);

extern const sivmfnptr_t sivmfn_primitives[];

#define SIVMFN_PRIMITIVE_COUNT (92)

sinanbox_t siexec(const svm_function_t *fn);

#define SISTATE_CURADDR (sistate.pc - sistate.program)
#define SISTATE_ADDRTOPC(addr) (sistate.program + (addr))

#ifdef __cplusplus
}
#endif

#endif // SINTER_VM_H
