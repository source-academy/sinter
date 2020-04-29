#ifndef SINTER_VM_H
#define SINTER_VM_H

#include "config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nanbox.h"
#include "heap_obj.h"
#include "opcode.h"
#include "fault.h"
#include "../sinter.h"

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

extern const sivmfnptr_t *sivmfn_vminternals;
extern size_t sivmfn_vminternal_count;

sinanbox_t siexec(const svm_function_t *fn, siheap_env_t *parent_env, uint8_t argc, sinanbox_t *argv);

SINTER_INLINE sinanbox_t siexec_nanbox(sinanbox_t fn, uint8_t argc, sinanbox_t *argv) {
  if (NANBOX_ISIFN(fn)) {
    uint8_t ifn = NANBOX_IFN_NUMBER(fn);
    if (NANBOX_IFN_TYPE(fn) && ifn < sivmfn_vminternal_count) {
      // vm-internal function
      return sivmfn_vminternals[ifn](argc, argv);
    } else if (!NANBOX_IFN_TYPE(fn) && ifn < SIVMFN_PRIMITIVE_COUNT) {
      return sivmfn_primitives[ifn](argc, argv);
    } else {
      sifault(sinter_fault_invalid_program);
      return NANBOX_OFEMPTY();
    }
  } else if (NANBOX_ISPTR(fn)) {
    siheap_header_t *v = (siheap_header_t *) SIHEAP_NANBOXTOPTR(fn);
    siheap_function_t *f = (siheap_function_t *) v;
    if (v->type != sitype_function) {
      sifault(sinter_fault_type);
      return NANBOX_OFEMPTY();
    }
    return siexec(f->code, f->env, argc, argv);
  } else {
    sifault(sinter_fault_type);
    return NANBOX_OFEMPTY();
  }
}

bool sivm_equal(sinanbox_t l, sinanbox_t r);

#define SISTATE_CURADDR (sistate.pc - sistate.program)
#define SISTATE_ADDRTOPC(addr) (sistate.program + (addr))

#ifndef __cplusplus
#define SIVMFN_PRINTFN(v) (_Generic((v), \
  char *: sinter_printer_string, \
  const char *: sinter_printer_string, \
  float: sinter_printer_float, \
  int32_t: sinter_printer_integer))
#define SIVMFN_PRINT(v, is_error) do { \
  if (SIVMFN_PRINTFN(v)) SIVMFN_PRINTFN(v)((v), (is_error)); \
} while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif // SINTER_VM_H
