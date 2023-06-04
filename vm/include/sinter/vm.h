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

SINTER_INLINEIFC __attribute__((warn_unused_result)) sinanbox_t siexec_nanbox(sinanbox_t fn, uint8_t argc, sinanbox_t *argv);
#ifndef __cplusplus
SINTER_INLINEIFC __attribute__((warn_unused_result)) sinanbox_t siexec_nanbox(sinanbox_t fn, uint8_t argc, sinanbox_t *argv) {
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
    switch (v->type) {
    case sitype_function: {
      siheap_function_t *f = (siheap_function_t *) v;
      return siexec(f->code, f->env, argc, argv);
    }
    case sitype_intcont: {
      siheap_intcont_t *f = (siheap_intcont_t *) v;
      // no need to deref arguments; it is handled when the intcont is destroyed
      return f->fn(f->argc, f->argv);
    }
    case sitype_empty:
    case sitype_frame:
    case sitype_env:
    case sitype_strconst:
    case sitype_strpair:
    case sitype_string:
    case sitype_array:
    case sitype_array_data:
    case sitype_free:
    default:
      sifault(sinter_fault_type);
      return NANBOX_OFEMPTY();
    }
  } else {
    sifault(sinter_fault_type);
    return NANBOX_OFEMPTY();
  }
}
#endif

bool sivm_equal(sinanbox_t l, sinanbox_t r);

void sistop(void);

#define SISTATE_CURADDR (sistate.pc - sistate.program)
#define SISTATE_ADDRTOPC(addr) (sistate.program + (addr))

#ifdef __cplusplus
}
#endif

#endif // SINTER_VM_H
