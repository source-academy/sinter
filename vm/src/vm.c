#include <sinter/config.h>

#include <math.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <sinter.h>

#include <sinter/opcode.h>
#include <sinter/fault.h>
#include <sinter/nanbox.h>
#include <sinter/heap.h>
#include <sinter/heap_obj.h>
#include <sinter/vm.h>
#include <sinter/stack.h>
#include <sinter/debug.h>
#include <sinter/program.h>

struct sistate sistate;

const sivmfnptr_t *sivmfn_vminternals = NULL;
size_t sivmfn_vminternal_count = 0;

sinter_printfn_string sinter_printer_string = NULL;
sinter_printfn_integer sinter_printer_integer = NULL;
sinter_printfn_float sinter_printer_float = NULL;
sinter_printfn_flush sinter_printer_flush = NULL;

#if 0
static inline void unimpl_instr() {
  SIBUGV("Unimplemented instruction %02x at address 0x%tx\n", *sistate.pc, SISTATE_CURADDR);
  sifault(sinter_fault_invalid_program);
}
#endif

bool sivm_equal(sinanbox_t l, sinanbox_t r) {
  if (NANBOX_GETTYPE(l) == NANBOX_GETTYPE(r) && NANBOX_IDENTICAL(l, r)) {
    // if they are *identical* then they are equal provided they are not NaN
    return !NANBOX_IDENTICAL(l, NANBOX_CANONICAL_NAN);
  } else if (NANBOX_ISNUMERIC(l) && NANBOX_ISNUMERIC(r)) {
    switch (NANBOX_ISFLOAT(r) << 1 | NANBOX_ISFLOAT(l)) {
      case 0: /* neither are floats */
        return NANBOX_INT(l) == NANBOX_INT(r);
      case 1: /* v0 is float */
        return NANBOX_FLOAT(l) == NANBOX_INT(r);
      case 2: /* r is float */
        return NANBOX_INT(l) == NANBOX_FLOAT(r);
      case 3: /* both are float */
        return NANBOX_FLOAT(l) == NANBOX_FLOAT(r);
      default:
        SIBUG();
        sifault(sinter_fault_internal_error);
        return false;
    }
  } else if (NANBOX_ISPTR(l) & NANBOX_ISPTR(r)) {
    siheap_header_t *hv0 = SIHEAP_NANBOXTOPTR(l);
    siheap_header_t *hv1 = SIHEAP_NANBOXTOPTR(r);
    if (siheap_is_string(hv0) && siheap_is_string(hv1)) {
      return strcmp(sistrobj_tocharptr(hv0), sistrobj_tocharptr(hv1)) == 0;
    } else {
      // for arrays and functions, identical only if they are the SAME object
      return hv0 == hv1;
    }
  } else {
    // different types, so not equal
    return false;
  }
}

static inline void pop_array_args(siheap_array_t **array, address_t *index) {
  sinanbox_t indexv = sistack_pop();
  sinanbox_t arrayv = sistack_pop();
  *array = SIHEAP_NANBOXTOPTR(arrayv);

  if (!NANBOX_ISPTR(arrayv) || (*array)->header.type != sitype_array) {
    sifault(sinter_fault_type);
    return;
  }

  if (NANBOX_ISINT(indexv)) {
    int32_t t = NANBOX_INT(indexv);
    if (t < 0) {
      sifault(sinter_fault_invalid_load);
      return;
    }
    *index = (address_t) t;
  } else if (NANBOX_ISFLOAT(indexv)) {
    // TODO check if float is integral
    float t = (address_t) NANBOX_FLOAT(indexv);
    if (t < 0) {
      sifault(sinter_fault_invalid_load);
      return;
    }
    *index = (address_t) t;
  }
}

static inline bool do_internal_function(
  const uint8_t id,
  const uint8_t num_args,
  size_t sizeof_instr,
  const bool is_primitive,
  const bool is_tailcall,
  const bool pop_fn) {
  if ((is_primitive && id >= SIVMFN_PRIMITIVE_COUNT) || (!is_primitive && id >= sivmfn_vminternal_count)) {
    SIDEBUG("Invalid %s function index %d\n", is_primitive ? "primitive" : "VM-internal", id);
    sifault(sinter_fault_invalid_program);
    return false;
  }

  // check that there are enough items on the stack
  if (num_args > 0) {
    sistack_peek(num_args - 1);
  }

  // call the function
  sinanbox_t retv = (is_primitive ? sivmfn_primitives : sivmfn_vminternals)[id](num_args, sistack_top - num_args);

  // pop the arguments off the stack
  for (unsigned int i = 0; i < num_args; ++i) {
    siheap_derefbox(sistack_pop());
  }

  // pop the function off the stack, if needed
  if (pop_fn) {
    siheap_derefbox(sistack_pop());
  }

  // if tail call, we destroy the caller's stack now, and "return" to the caller's caller
  if (is_tailcall) {
    siheap_deref(sistate.env);
    sistack_destroy(&sistate.pc, &sistate.env);
  } else {
    // otherwise we advance to the return address
    sistate.pc += sizeof_instr;
  }

  sistack_push(retv);

  // tail call from main
  if (is_tailcall && !sistate.pc) {
    return true;
  }

  return false;
}

#define DECLOPSTRUCT(type) const struct type *instr = (const struct type *) sistate.pc
#define ADVANCE_PCONE() sistate.pc += sizeof(opcode_t); continue
#define ADVANCE_PCI() sistate.pc += sizeof(*instr); continue

/**
 * Runs the main interpreter loop.
 */
static void main_loop(void) {
#ifdef SINTER_DEBUG
  const opcode_t *previous_pc = NULL;
  (void) previous_pc;
#endif
  while (1) {
#ifdef SINTER_DEBUG_MEMORY_CHECK
    debug_memorycheck();
#endif
#ifdef SINTER_DEBUG
    if (sistate.pc >= sistate.program_end) {
      SIBUGV("Jumped out of bounds to 0x%tx after instruction at address 0x%tx\n", SISTATE_CURADDR, previous_pc - sistate.program);
      sifault(sinter_fault_internal_error);
      return;
    }
    previous_pc = sistate.pc;

    SITRACE("PC: 0x%tx; opcode: %02x (%s)\n", SISTATE_CURADDR, *sistate.pc, get_opcode_name(*sistate.pc));
#endif
    const opcode_t this_opcode = *sistate.pc;
    switch (this_opcode) {
    case op_nop:
      ADVANCE_PCONE();
    case op_ldc_i:
    case op_lgc_i: {
      DECLOPSTRUCT(op_i32);
      sistack_push(NANBOX_WRAP_INT(instr->operand));
      ADVANCE_PCI();
    }
    case op_ldc_f32:
    case op_lgc_f32: {
      DECLOPSTRUCT(op_f32);
      sistack_push(NANBOX_OFFLOAT(instr->operand));
      ADVANCE_PCI();
    }
    case op_ldc_f64:
    case op_lgc_f64: {
      DECLOPSTRUCT(op_f64);
#ifdef SINTER_SHORT_DOUBLE_WORKAROUND
      // for systems (e.g. Arduino AVR) where double is actually an alias of float...
      // manually convert the double into a float
      union {
        float value;
        uint32_t bits;
      } float_value;
      _Static_assert(sizeof(float_value) == 4, "union of float and uint32_t is not 32-bit");

      float_value.bits = 0;
      // sign bit
      if (instr->operand_u64 & (((uint64_t)1) << 63)) {
        float_value.bits |= ((uint32_t)1) << 31;
      }

      const uint32_t offset_exponent = (instr->operand_u64 >> 52) & 0x7FFu;
      const int32_t real_exponent = ((int32_t)offset_exponent) - 1023;
      const uint64_t f64_mantissa = instr->operand_u64 & 0xFFFFFFFFFFFFFu;
      // lop off the bottom 29 bits..
      const uint32_t f32_mantissa = (instr->operand_u64 >> 29) & 0x7FFFFFu;
      if (offset_exponent == 0x7FF) {
        // NaN or infinity
        // set all the exponent bits
        float_value.bits |= 0x7F800000u;
        if (f64_mantissa) {
          // NaN, just set this to canonical NaN
          float_value.bits = 0x7FC00000u;
        }
      } else if (offset_exponent == 0) {
        // zero/subnormal
        float_value.bits |= f32_mantissa;
      } else if (real_exponent >= -126 && real_exponent <= 127) {
        float_value.bits |= (real_exponent + 127) << 23;
        float_value.bits |= f32_mantissa;
      } else if (real_exponent < -126) {
        float_value.value = -INFINITY;
      } else if (real_exponent > 127) {
        float_value.value = INFINITY;
      }
      sistack_push(NANBOX_OFFLOAT(float_value.value));
#else
      sistack_push(NANBOX_OFFLOAT((float)instr->operand));
#endif
      ADVANCE_PCI();
    }
    case op_ldc_b_0:
    case op_lgc_b_0:
      sistack_push(NANBOX_OFBOOL(false));
      ADVANCE_PCONE();
    case op_ldc_b_1:
    case op_lgc_b_1:
      sistack_push(NANBOX_OFBOOL(true));
      ADVANCE_PCONE();
    case op_lgc_u:
      sistack_push(NANBOX_OFUNDEF());
      ADVANCE_PCONE();
    case op_lgc_n:
      sistack_push(NANBOX_OFNULL());
      ADVANCE_PCONE();
    case op_lgc_s: {
      DECLOPSTRUCT(op_address);
      const svm_constant_t *string = (const svm_constant_t *) (sistate.program + instr->address);
      siheap_strconst_t *obj = sistrconst_new(string);
      sistack_push(SIHEAP_PTRTONANBOX(obj));
      ADVANCE_PCI();
    }
    case op_pop_g:
    case op_pop_b:
    case op_pop_f:
      siheap_derefbox(sistack_pop());
      ADVANCE_PCONE();

#define ARITHMETIC_TYPECHECK() do { if (!NANBOX_ISNUMERIC(v0) || !NANBOX_ISNUMERIC(v1)) {\
  sifault(sinter_fault_type); \
  return; \
} } while (0)

    // TODO: optimised _f variants
    case op_add_g:
    case op_add_f: {
      sinanbox_t v1 = sistack_pop();
      sinanbox_t v0 = sistack_pop();
      sinanbox_t r;

      if (NANBOX_ISNUMERIC(v0) && NANBOX_ISNUMERIC(v1)) {
        switch (NANBOX_ISFLOAT(v1) << 1 | NANBOX_ISFLOAT(v0)) {
        case 0: /* neither are floats */
          /* addition/subtraction of 2 21-bit integers won't overflow a 32-bit integer; no worries here */
          r = NANBOX_WRAP_INT(NANBOX_INT(v0) + NANBOX_INT(v1));
          break;
        case 1: /* v0 is float */
          r = NANBOX_OFFLOAT(NANBOX_FLOAT(v0) + NANBOX_INT(v1));
          break;
        case 2: /* v1 is float */
          r = NANBOX_OFFLOAT(NANBOX_INT(v0) + NANBOX_FLOAT(v1));
          break;
        case 3: /* both are float */
          r = NANBOX_OFFLOAT(NANBOX_FLOAT(v0) + NANBOX_FLOAT(v1));
          break;
        default:
          SIBUG();
          sifault(sinter_fault_internal_error);
          break;
        }
      } else if (NANBOX_ISPTR(v0) & NANBOX_ISPTR(v1)) {
        siheap_header_t *hv0 = SIHEAP_NANBOXTOPTR(v0);
        siheap_header_t *hv1 = SIHEAP_NANBOXTOPTR(v1);

        if (siheap_is_string(hv0) && siheap_is_string(hv1)) {
          // if either are empty string, no-op
          if (hv0->type == sitype_strconst && *(((siheap_strconst_t *) hv0)->string->data) == '\0') {
            siheap_ref(hv1);
            r = v1;
          } else if (hv1->type == sitype_strconst && *(((siheap_strconst_t *) hv1)->string->data) == '\0') {
            siheap_ref(hv0);
            r = v0;
          } else {
            siheap_strpair_t *obj = sistrpair_new(hv0, hv1);
            r = SIHEAP_PTRTONANBOX(obj);
          }
        } else {
          SIDEBUG("Invalid operands to add.\n");
          sifault(sinter_fault_type);
          return;
        }
      } else {
        SIDEBUG("Invalid operands to add.\n");
        sifault(sinter_fault_type);
        return;
      }

      sistack_push(r);
      siheap_derefbox(v0);
      siheap_derefbox(v1);
      ADVANCE_PCONE();
    }
    break;
    case op_sub_g:
    case op_sub_f: {
      sinanbox_t v1 = sistack_pop();
      sinanbox_t v0 = sistack_pop();
      ARITHMETIC_TYPECHECK();
      sinanbox_t r;
      switch (NANBOX_ISFLOAT(v1) << 1 | NANBOX_ISFLOAT(v0)) {
      case 0: /* neither are floats */
        /* addition/subtraction of 2 21-bit integers won't overflow a 32-bit integer; no worries here */
        r = NANBOX_WRAP_INT(NANBOX_INT(v0) - NANBOX_INT(v1));
        break;
      case 1: /* v0 is float */
        r = NANBOX_OFFLOAT(NANBOX_FLOAT(v0) - NANBOX_INT(v1));
        break;
      case 2: /* v1 is float */
        r = NANBOX_OFFLOAT(NANBOX_INT(v0) - NANBOX_FLOAT(v1));
        break;
      case 3: /* both are float */
        r = NANBOX_OFFLOAT(NANBOX_FLOAT(v0) - NANBOX_FLOAT(v1));
        break;
      default:
        SIBUG();
        sifault(sinter_fault_internal_error);
        return;
      }
      sistack_push(r);
      /* No need to deref v0 and v1; they are either numbers (which are not on the heap) */
      /* or they are not (in which case we would have faulted) */
      ADVANCE_PCONE();
    }

    case op_mul_g:
    case op_mul_f: {
      sinanbox_t v1 = sistack_pop();
      sinanbox_t v0 = sistack_pop();
      ARITHMETIC_TYPECHECK();
      sinanbox_t r;
      switch (NANBOX_ISFLOAT(v1) << 1 | NANBOX_ISFLOAT(v0)) {
      case 0: /* neither are floats */
        /* this can overflow, use int64 instead */
        r = NANBOX_WRAP_INT(((int64_t) NANBOX_INT(v0)) * ((int64_t) NANBOX_INT(v1)));
        break;
      case 1: /* v0 is float */
        r = NANBOX_OFFLOAT(NANBOX_FLOAT(v0) *  NANBOX_INT(v1));
        break;
      case 2: /* v1 is float */
        r = NANBOX_OFFLOAT(NANBOX_INT(v0) * NANBOX_FLOAT(v1));
        break;
      case 3: /* both are float */
        r = NANBOX_OFFLOAT(NANBOX_FLOAT(v0) * NANBOX_FLOAT(v1));
        break;
      default:
        SIBUG();
        sifault(sinter_fault_internal_error);
        return;
      }
      sistack_push(r);
      /* No need to deref v0 and v1; they are either numbers (which are not on the heap) */
      /* or they are not (in which case we would have faulted) */
      ADVANCE_PCONE();
    }

    case op_div_g:
    case op_div_f: {
      sinanbox_t v1 = sistack_pop();
      sinanbox_t v0 = sistack_pop();
      ARITHMETIC_TYPECHECK();
      sinanbox_t r;
      switch (NANBOX_ISFLOAT(v1) << 1 | NANBOX_ISFLOAT(v0)) {
      case 0: /* neither are floats */
        r = NANBOX_OFFLOAT(((float) NANBOX_INT(v0)) / NANBOX_INT(v1));
        break;
      case 1: /* v0 is float */
        r = NANBOX_OFFLOAT(NANBOX_FLOAT(v0) / NANBOX_INT(v1));
        break;
      case 2: /* v1 is float */
        r = NANBOX_OFFLOAT(NANBOX_INT(v0) / NANBOX_FLOAT(v1));
        break;
      case 3: /* both are float */
        r = NANBOX_OFFLOAT(NANBOX_FLOAT(v0) / NANBOX_FLOAT(v1));
        break;
      default:
        SIBUG();
        sifault(sinter_fault_internal_error);
        return;
      }
      sistack_push(r);
      /* No need to deref v0 and v1; they are either numbers (which are not on the heap) */
      /* or they are not (in which case we would have faulted) */
      ADVANCE_PCONE();
    }

    case op_mod_g:
    case op_mod_f: {
      sinanbox_t v1 = sistack_pop();
      sinanbox_t v0 = sistack_pop();
      ARITHMETIC_TYPECHECK();
      sinanbox_t r;
      switch (NANBOX_ISFLOAT(v1) << 1 | NANBOX_ISFLOAT(v0)) {
      case 0: /* neither are floats */
        r = NANBOX_OFFLOAT(fmodf(NANBOX_INT(v0),  NANBOX_INT(v1)));
        break;
      case 1: /* v0 is float */
        r = NANBOX_OFFLOAT(fmodf(NANBOX_FLOAT(v0), NANBOX_INT(v1)));
        break;
      case 2: /* v1 is float */
        r = NANBOX_OFFLOAT(fmodf(NANBOX_INT(v0), NANBOX_FLOAT(v1)));
        break;
      case 3: /* both are float */
        r = NANBOX_OFFLOAT(fmodf(NANBOX_FLOAT(v0), NANBOX_FLOAT(v1)));
        break;
      default:
        SIBUG();
        sifault(sinter_fault_internal_error);
        break;
      }
      sistack_push(r);
      /* No need to deref v0 and v1; they are either numbers (which are not on the heap) */
      /* or they are not (in which case we would have faulted) */
      ADVANCE_PCONE();
    }

    case op_neg_g:
    case op_neg_f: {
      sinanbox_t v1 = sistack_pop();

      if (NANBOX_ISINT(v1)) {
        sistack_push(NANBOX_WRAP_INT(-NANBOX_INT(v1)));
      } else if (NANBOX_ISFLOAT(v1)) {
        sistack_push(NANBOX_OFFLOAT(-NANBOX_FLOAT(v1)));
      } else {
        sifault(sinter_fault_type);
        return;
      }

      ADVANCE_PCONE();
    }

    case op_not_g:
    case op_not_b: {
      sinanbox_t v = sistack_pop();
      if (!NANBOX_ISBOOL(v)) {
        sifault(sinter_fault_type);
        return;
      }
      sistack_push(NANBOX_OFBOOL(!NANBOX_BOOL(v)));
      ADVANCE_PCONE();
    }

#define COMPARISON_OP(op) { \
      sinanbox_t v1 = sistack_pop(); \
      sinanbox_t v0 = sistack_pop(); \
      sinanbox_t r; \
 \
      if (NANBOX_ISNUMERIC(v0) && NANBOX_ISNUMERIC(v1)) { \
        switch (NANBOX_ISFLOAT(v1) << 1 | NANBOX_ISFLOAT(v0)) { \
        case 0: /* neither are floats */ \
          r = NANBOX_OFBOOL(NANBOX_INT(v0) op NANBOX_INT(v1)); \
          break; \
        case 1: /* v0 is float */ \
          r = NANBOX_OFBOOL(NANBOX_FLOAT(v0) op NANBOX_INT(v1)); \
          break; \
        case 2: /* v1 is float */ \
          r = NANBOX_OFBOOL(NANBOX_INT(v0) op NANBOX_FLOAT(v1)); \
          break; \
        case 3: /* both are float */ \
          r = NANBOX_OFBOOL(NANBOX_FLOAT(v0) op NANBOX_FLOAT(v1)); \
          break; \
        default: \
          SIBUG(); \
          sifault(sinter_fault_internal_error); \
          break; \
        } \
      } else if (NANBOX_ISPTR(v0) & NANBOX_ISPTR(v1)) { \
        siheap_header_t *hv0 = SIHEAP_NANBOXTOPTR(v0); \
        siheap_header_t *hv1 = SIHEAP_NANBOXTOPTR(v1); \
        if (siheap_is_string(hv0) && siheap_is_string(hv1)) { \
          r = NANBOX_OFBOOL(strcmp(sistrobj_tocharptr(hv0), sistrobj_tocharptr(hv1)) op 0); \
        } else { \
          SIDEBUG("Invalid operands to comparison.\n"); \
          sifault(sinter_fault_type); \
          return; \
        } \
      } else { \
        SIDEBUG("Invalid operands to comparison.\n"); \
        sifault(sinter_fault_type); \
        return; \
      } \
 \
      sistack_push(r); \
      siheap_derefbox(v0); \
      siheap_derefbox(v1); \
      ADVANCE_PCONE(); \
    }

    case op_lt_g:
    case op_lt_f:
      COMPARISON_OP(<)
    case op_gt_g:
    case op_gt_f:
      COMPARISON_OP(>)
    case op_le_g:
    case op_le_f:
      COMPARISON_OP(<=)
    case op_ge_g:
    case op_ge_f:
      COMPARISON_OP(>=)
    case op_neq_g:
    case op_neq_f:
    case op_neq_b:
    case op_eq_g:
    case op_eq_f:
    case op_eq_b: {
      sinanbox_t v0 = sistack_pop();
      sinanbox_t v1 = sistack_pop();
      bool r = sivm_equal(v1, v0);

      if (this_opcode >= op_neq_g) {
        r = !r;
      }

      sistack_push(NANBOX_OFBOOL(r));
      siheap_derefbox(v0);
      siheap_derefbox(v1);
      ADVANCE_PCONE();
    }

    case op_new_c: {
      DECLOPSTRUCT(op_address);
      const svm_function_t *fn_code = (const svm_function_t *) SISTATE_ADDRTOPC(instr->address);
      siheap_function_t *fn_obj = sifunction_new(fn_code, sistate.env);
      sistack_push(SIHEAP_PTRTONANBOX(fn_obj));
      ADVANCE_PCI();
    }

    case op_new_c_p: {
      DECLOPSTRUCT(op_oneindex);
      sistack_push(NANBOX_OFIFN_PRIMITIVE(instr->index));
      ADVANCE_PCI();
    }

    case op_new_c_v: {
      DECLOPSTRUCT(op_oneindex);
      sistack_push(NANBOX_OFIFN_VM(instr->index));
      ADVANCE_PCI();
    }

    case op_new_a: {
      siheap_array_t *array = siarray_new(8);
      sistack_push(SIHEAP_PTRTONANBOX(array));
      ADVANCE_PCONE();
    }

    case op_ldl_g:
    case op_ldl_f:
    case op_ldl_b: {
      DECLOPSTRUCT(op_oneindex);
      sinanbox_t v = sienv_get(sistate.env, instr->index);
      if (NANBOX_ISEMPTY(v)) {
        sifault(sinter_fault_uninitialised_load);
        return;
      }
      siheap_refbox(v);
      sistack_push(v);
      ADVANCE_PCI();
    }

    case op_stl_g:
    case op_stl_b:
    case op_stl_f: {
      DECLOPSTRUCT(op_oneindex);
      sinanbox_t v = sistack_pop();
      sienv_put(sistate.env, instr->index, v);
      ADVANCE_PCI();
    }

    case op_ldp_g:
    case op_ldp_f:
    case op_ldp_b: {
      DECLOPSTRUCT(op_twoindex);
      siheap_env_t *env = sienv_getparent(sistate.env, instr->envindex);
      if (!env) {
        sifault(sinter_fault_invalid_load);
        return;
      }
      sinanbox_t v = sienv_get(env, instr->index);
      if (NANBOX_ISEMPTY(v)) {
        sifault(sinter_fault_uninitialised_load);
        return;
      }
      siheap_refbox(v);
      sistack_push(v);
      ADVANCE_PCI();
    }

    case op_stp_g:
    case op_stp_b:
    case op_stp_f: {
      DECLOPSTRUCT(op_twoindex);
      siheap_env_t *env = sienv_getparent(sistate.env, instr->envindex);
      if (!env) {
        sifault(sinter_fault_invalid_load);
        return;
      }
      sinanbox_t v = sistack_pop();
      sienv_put(env, instr->index, v);
      ADVANCE_PCI();
    }

    case op_lda_g:
    case op_lda_b:
    case op_lda_f: {
      siheap_array_t *array = NULL;
      address_t index = 0;
      pop_array_args(&array, &index);

      sinanbox_t loadv = siarray_get(array, index);
      siheap_refbox(loadv);
      siheap_deref(array);

      sistack_push(loadv);

      ADVANCE_PCONE();
    }

    case op_sta_g:
    case op_sta_b:
    case op_sta_f: {
      sinanbox_t storev = sistack_pop();
      siheap_array_t *array = NULL;
      address_t index = 0;
      pop_array_args(&array, &index);

      siarray_put(array, index, storev);
      siheap_deref(array);

      ADVANCE_PCONE();
    }

    case op_br_t:
    case op_br_f: {
      DECLOPSTRUCT(op_offset);
      sinanbox_t v = sistack_pop();
      if (!NANBOX_ISBOOL(v)) {
        sifault(sinter_fault_type);
        return;
      }
      if (NANBOX_BOOL(v) == (this_opcode == op_br_t)) {
        sistate.pc += instr->offset + sizeof(*instr);
        break;
      } else {
        ADVANCE_PCI();
      }
    }

    case op_br: {
      DECLOPSTRUCT(op_offset);
      sistate.pc += instr->offset + sizeof(*instr);
      break;
    }

    case op_jmp: {
      DECLOPSTRUCT(op_address);
      sistate.pc = SISTATE_ADDRTOPC(instr->address);
      break;
    }

    case op_call:
    case op_call_t: {
      // There are three types of functions:
      // - regular SVM closures (those created by new.c)
      // - internal functions (represented in a NaNbox)
      // - internal continuations (used for streams)
      // each of them have slightly different ways to call them, so you end up
      // with three slightly different variants of the function call code
      DECLOPSTRUCT(op_call);

      // get the function object
      sinanbox_t fn_ptr = sistack_peek(instr->num_args);
      const bool is_tailcall = this_opcode == op_call_t;

      if (NANBOX_ISIFN(fn_ptr)) {
        if (do_internal_function(NANBOX_IFN_NUMBER(fn_ptr), instr->num_args, sizeof(*instr), NANBOX_IFN_TYPE(fn_ptr) == 0, is_tailcall, true)) {
          return;
        }
      } else if (NANBOX_ISPTR(fn_ptr)) {
        siheap_header_t *obj = SIHEAP_NANBOXTOPTR(fn_ptr);
        if (obj->type == sitype_function) {
          siheap_function_t *fn_obj = (siheap_function_t *) obj;

          // get the code
          const svm_function_t *fn_code = fn_obj->code;

          if (instr->num_args != fn_code->num_args) {
            sifault(sinter_fault_function_arity);
            return;
          }

          if (fn_code->num_args > fn_code->env_size) {
            sifault(sinter_fault_invalid_load);
            return;
          }

          // create the new environment
          siheap_env_t *new_env = sienv_new(fn_obj->env, fn_code->env_size);

          // check we have enough arguments on the stack
          sistack_top -= fn_code->num_args;
          if (sistack_top < sistack_bottom) {
            sifault(sinter_fault_stack_underflow);
            return;
          }

          // copy the arguments from the stack to the environment
          memcpy(new_env->entry, sistack_top, fn_code->num_args*sizeof(sinanbox_t));

          // pop the function off the caller's stack, and deref it at the same time
          siheap_derefbox(sistack_pop());


          // if tail call, we destroy the caller's stack now, and "return" to the caller's caller
          if (is_tailcall) {
            siheap_deref(sistate.env);
            sistack_destroy(&sistate.pc, &sistate.env);
          } else {
            // otherwise we advance to the return address
            sistate.pc += sizeof(*instr);
          }

          // create the stack frame for the callee, which stores the return address and environment
          sistack_new(fn_code->stack_size, sistate.pc, sistate.env);

          // set the environment
          sistate.env = new_env;

          // enter the function
          sistate.pc = &fn_code->code;
        } else if (obj->type == sitype_intcont) {
          siheap_intcont_t *fn_obj = (siheap_intcont_t *) obj;

          // continuations are zero-arity
          if (instr->num_args) {
            sifault(sinter_fault_function_arity);
            return;
          }

          // call the function
          sinanbox_t retv = fn_obj->fn(fn_obj->argc, fn_obj->argv);

          // pop the function off the stack
          // note: we've checked for arity above, there should be 0 arguments
          siheap_derefbox(sistack_pop());

          // if tail call, we destroy the caller's stack now, and "return" to the caller's caller
          if (is_tailcall) {
            siheap_deref(sistate.env);
            sistack_destroy(&sistate.pc, &sistate.env);
          } else {
            // otherwise we advance to the return address
            sistate.pc += sizeof(*instr);
          }

          sistack_push(retv);

          // tail call from main
          if (is_tailcall && !sistate.pc) {
            return;
          }
        } else {
          sifault(sinter_fault_type);
          return;
        }
      }
      break;
    }

    case op_call_v:
    case op_call_t_v:
    case op_call_p:
    case op_call_t_p: {
      DECLOPSTRUCT(op_call_internal);
      const bool is_primitive = this_opcode == op_call_p || this_opcode == op_call_t_p;
      const bool is_tailcall = this_opcode == op_call_t_v || this_opcode == op_call_t_p;

      if (do_internal_function(instr->id, instr->num_args, sizeof(*instr), is_primitive, is_tailcall, false)) {
        return;
      }

      break;
    }

    case op_ret_g:
    case op_ret_f:
    case op_ret_b: {
      // pop the return value
      sinanbox_t v = sistack_pop();

      // destroy this stack frame, and return to the caller
      siheap_deref(sistate.env);
      sistack_destroy(&sistate.pc, &sistate.env);

      // push the return value onto the caller's stack
      sistack_push(v);

      // return from top-level (main); exit loop
      if (!sistate.pc) {
        return;
      }

      break;
    }

    case op_ret_u:
    case op_ret_n:
      // destroy this stack frame, and return to the caller
      siheap_deref(sistate.env);
      sistack_destroy(&sistate.pc, &sistate.env);

      // push the return value onto the caller's stack
      if (this_opcode == op_ret_u) {
        sistack_push(NANBOX_OFUNDEF());
      } else {
        sistack_push(NANBOX_OFNULL());
      }

      // return from top-level (main); exit loop
      if (!sistate.pc) {
        return;
      }

      break;

    case op_dup: {
      sinanbox_t v = sistack_peek(0);
      siheap_refbox(v);
      sistack_push(v);
      ADVANCE_PCONE();
    }

    case op_newenv: {
      DECLOPSTRUCT(op_oneindex);
      siheap_env_t *new_env = sienv_new(sistate.env, instr->index);
      siheap_deref(sistate.env);
      sistate.env = new_env;
      ADVANCE_PCI();
    }

    case op_popenv: {
      siheap_env_t *old_env = sistate.env;
      sistate.env = old_env->parent;
      siheap_ref(sistate.env);
      siheap_deref(old_env);
      ADVANCE_PCONE();
    }

    default:
      SIBUGV("Invalid instruction %02x at address 0x%tx\n", this_opcode, SISTATE_CURADDR);
      sifault(sinter_fault_invalid_program);
      break;
    }

  }
}

/**
 * Executes an SVM function.
 *
 * This is used by the main entrypoint in main.c, as well as by primitive
 * functions that need to execute functions given to it (e.g. map).
 */
sinanbox_t siexec(const svm_function_t *fn, siheap_env_t *parent_env, uint8_t argc, sinanbox_t *argv) {
  siheap_env_t *old_env = sistate.env;
  const opcode_t *old_pc = sistate.pc;

  if (fn->env_size < argc) {
    sifault(sinter_fault_invalid_load);
    return NANBOX_OFEMPTY();
  }

  sistack_limit++; // create one entry for the return value
  sistate.env = sienv_new(parent_env, fn->env_size);
  sistack_new(fn->stack_size, NULL, old_env);
  if (argc) {
    memcpy(sistate.env->entry, argv, argc*sizeof(sinanbox_t));
  }
  sistate.pc = &fn->code;

  main_loop();

  sinanbox_t ret = sistack_top == sistack_bottom ? NANBOX_OFEMPTY() : *(--sistack_top);
  sistate.env = old_env;
  sistate.pc = old_pc;
  sistack_limit--;

  return ret;
}
