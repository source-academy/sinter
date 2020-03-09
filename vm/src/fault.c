#include "fault.h"
#include "vm.h"

jmp_buf sinter_fault_jmp = { 0 };

_Noreturn void sifault(enum sinter_fault reason) {
  sistate.fault_reason = reason;
  longjmp(sinter_fault_jmp, 1);
}
