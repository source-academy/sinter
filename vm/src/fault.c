#include <assert.h>

#include <sinter/fault.h>
#include <sinter/vm.h>

jmp_buf sinter_fault_jmp = { 0 };

_Noreturn void sifault(sinter_fault_t reason) {
  sistate.fault_reason = reason;
  assert("Faulting." == 0);
  longjmp(sinter_fault_jmp, 1);
}
