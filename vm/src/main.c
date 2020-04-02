#include <sinter.h>

#include "stack.h"
#include "program.h"
#include "vm.h"

static inline void validate_header(const struct svm_header *const header) {
  if (header->magic != SVM_MAGIC) {
    SIDEBUG("Invalid magic: %x\n", header->magic);
    sifault(sinter_fault_invalid_program);
    return;
  }
}

static void set_result(sinanbox_t exec_result, struct sinter_value *result) {
  if (NANBOX_ISEMPTY(exec_result)) {
    SIDEBUG("Program did not return value from toplevel\n");
    return;
  }

  SIDEBUG("Return value: ");
  SIDEBUG_NANBOX(exec_result);
  SIDEBUG("\n");
  switch (NANBOX_GETTYPE(exec_result)) {
  NANBOX_CASES_TINT
    result->type = sinter_type_integer;
    result->integer_value = NANBOX_INT(exec_result);
    break;
  case NANBOX_TBOOL:
    result->type = sinter_type_boolean;
    result->boolean_value = NANBOX_BOOL(exec_result);
    break;
  case NANBOX_TUNDEF:
    result->type = sinter_type_undefined;
    break;
  case NANBOX_TNULL:
    result->type = sinter_type_null;
    break;
  default:
    if (NANBOX_ISFLOAT(exec_result)) {
      result->type = sinter_type_float;
      result->float_value = NANBOX_FLOAT(exec_result);
    } else {
      SIBUGM("Unexpected return type\n");
    }
    break;
  }
}

enum sinter_fault sinter_run(const unsigned char *const code, const size_t code_size, struct sinter_value *result) {
  sistate.fault_reason = sinter_fault_none;
  sistate.program = code;
  sistate.program_end = code + code_size;
  sistate.running = true;
  sistate.pc = NULL;
  sistate.env = NULL;

  if (SINTER_FAULTED()) {
    *result = (struct sinter_value) { 0 };
    return sistate.fault_reason;
  }

  // Reset the heap and stack
  siheap_init();
  sistack_init();

  // Create one entry for the return value of the entrypoint
  sistack_limit++;

  const struct svm_header *header = (const struct svm_header *) code;
  validate_header(header);

  const struct svm_function *entry_fn = (const struct svm_function *) SISTATE_ADDRTOPC(header->entry);
  sinanbox_t exec_result = siexec(entry_fn);
  set_result(exec_result, result);

  return sinter_fault_none;
}
