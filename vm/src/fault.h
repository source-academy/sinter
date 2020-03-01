#ifndef SINTER_FAULT_H
#define SINTER_FAULT_H

#include <setjmp.h>

#include <sinter.h>

extern jmp_buf sifault_jmp;

_Noreturn void sifault(enum sinter_fault);

#define SINTER_FAULTED() setjmp(sifault_jmp)

#endif
