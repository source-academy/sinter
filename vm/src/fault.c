#include "fault.h"

jmp_buf sinter_fault_jmp = { 0 };

_Noreturn void sinter_fault(enum sinter_fault reason) {
  longjmp(sinter_fault_jmp, reason);
}
