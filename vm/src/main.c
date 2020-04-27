#include <sinter/config.h>

#include <sinter.h>

#include <sinter/heap.h>
#include <sinter/stack.h>
#include <sinter/program.h>
#include <sinter/vm.h>

/**
 * Validates the program header. Faults if it is invalid.
 */
static inline void validate_header(const svm_header_t *const header) {
  if (header->magic != SVM_MAGIC) {
    SIDEBUG("Invalid magic: %x\n", header->magic);
    sifault(sinter_fault_invalid_program);
    return;
  }
}

/**
 * Expands our sinanbox_t result into a friendlier structure for external
 * consumers to use.
 */
static void set_result(sinanbox_t exec_result, sinter_value_t *result) {
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
  case NANBOX_TIFN:
    result->type = sinter_type_function;
    break;
  NANBOX_CASES_TPTR {
    siheap_header_t *obj = SIHEAP_NANBOXTOPTR(exec_result);
    switch (obj->type) {
    case sitype_strconst:
    case sitype_string:
    case sitype_strpair:
      result->type = sinter_type_string;
      result->string_value = sistrobj_tocharptr(obj);
      break;
    case sitype_function:
      result->type = sinter_type_function;
      break;
    case sitype_array_data:
    case sitype_empty:
    case sitype_frame:
    case sitype_free:
    case sitype_marked:
    case sitype_env:
    case sitype_array:
    default:
      SIBUGV("Unexpected return object type %d\n", obj->type);
      break;
    }
    break;
  }
  default:
    if (NANBOX_ISFLOAT(exec_result)) {
      result->type = sinter_type_float;
      result->float_value = NANBOX_FLOAT(exec_result);
    } else {
      SIBUGV("Unexpected NaNbox: %08x\n", exec_result.as_i32);
    }
    break;
  }
}

sinter_fault_t sinter_run(const unsigned char *const code, const size_t code_size, sinter_value_t *result) {
#ifndef SINTER_STATIC_HEAP
  if (!siheap) {
    SIDEBUG("Heap not yet initialised!\n");
    return sinter_fault_uninitialised_heap;
  }
#endif

  sistate.fault_reason = sinter_fault_none;
  sistate.program = code;
  sistate.program_end = code + code_size;
  sistate.running = true;
  sistate.pc = NULL;
  sistate.env = NULL;

  if (SINTER_FAULTED()) {
    *result = (sinter_value_t) { 0 };
    return sistate.fault_reason;
  }

  // Reset the heap and stack
  siheap_init();
  sistack_init();

  // Create one entry for the return value of the entrypoint
  sistack_limit++;

  const svm_header_t *header = (const svm_header_t *) code;
  validate_header(header);

  const svm_function_t *entry_fn = (const svm_function_t *) SISTATE_ADDRTOPC(header->entry);
  sinanbox_t exec_result = siexec(entry_fn);
  set_result(exec_result, result);

  return sinter_fault_none;
}

void sinter_setup_heap(void *heap, size_t size) {
#ifdef SINTER_STATIC_HEAP
(void) heap; (void) size;
return;
#else
  siheap = heap;
  siheap_size = size;
#endif
}
