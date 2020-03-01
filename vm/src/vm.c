#include <sinter.h>

#include "opcode.h"
#include "vm.h"
#include "fault.h"
#include "nanbox.h"

struct sistate sistate;

enum sinter_fault sinter_run(const unsigned char *code, struct sinter_value *result) {
  (void) code;
  (void) result;

  // TODO
  return sinter_fault_none;
}
