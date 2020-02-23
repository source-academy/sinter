#ifndef SINTER_FAULT_H
#define SINTER_FAULT_H

#include <setjmp.h>

extern jmp_buf sinter_fault_jmp;

enum sinter_fault {
  // skip 0 (setjmp returns 0 on normal return)
  sinter_fault_none = 1,
  sinter_fault_oom = 2,
  sinter_fault_type = 3,
  sinter_fault_divbyzero = 4,
  sinter_fault_stackoverflow = 5,
  sinter_fault_stackunderflow = 6,
  sinter_fault_uninitld = 7,
  sinter_fault_invalidld = 8
};

_Noreturn void sinter_fault(enum sinter_fault);

#define SINTER_FAULTED() setjmp(sinter_fault_jmp)

#endif
