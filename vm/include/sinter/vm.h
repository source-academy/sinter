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
#include "internal_fn.h"

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

sinanbox_t __attribute__((warn_unused_result)) siexec(const svm_function_t *fn, siheap_env_t *parent_env, uint8_t argc, sinanbox_t *argv);

SINTER_INLINE __attribute__((warn_unused_result)) sinanbox_t siexec_nanbox(sinanbox_t fn, uint8_t argc, sinanbox_t *argv) {
  if (NANBOX_ISIFN(fn)) {
    uint8_t ifn = NANBOX_IFN_NUMBER(fn);
    sinanbox_t ret = NANBOX_OFEMPTY();
    if (NANBOX_IFN_TYPE(fn) && ifn < sivmfn_vminternal_count) {
      // vm-internal function
      ret = sivmfn_vminternals[ifn](argc, argv);
    } else if (!NANBOX_IFN_TYPE(fn) && ifn < SIVMFN_PRIMITIVE_COUNT) {
      ret = sivmfn_primitives[ifn](argc, argv);
    } else {
      sifault(sinter_fault_invalid_program);
      return NANBOX_OFEMPTY();
    }
    for (size_t i = 0; i < argc; ++i) {
      siheap_derefbox(argv[i]);
    }
    return ret;
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

#ifdef __cplusplus
}
#endif

#endif // SINTER_VM_H
