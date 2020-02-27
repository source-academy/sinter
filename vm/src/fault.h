#ifndef SINTER_FAULT_H
#define SINTER_FAULT_H

#include <setjmp.h>

extern jmp_buf sinter_fault_jmp;

_Noreturn void sinter_fault(enum sinter_fault);

#define SINTER_FAULTED() setjmp(sinter_fault_jmp)

#endif
