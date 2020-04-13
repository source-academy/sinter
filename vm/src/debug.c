#include <stdio.h>
#include <inttypes.h>

#include <sinter/nanbox.h>
#include <sinter/debug.h>

#if SINTER_DEBUG_LEVEL >= 1
void debug_nanbox(sinanbox_t v) {
  switch (NANBOX_GETTYPE(v)) {
  NANBOX_CASES_TINT
    SIDEBUG(("integer, value: %" PRId32), NANBOX_INT(v));
    break;
  case NANBOX_TBOOL:
    SIDEBUG("boolean, value: %d", NANBOX_BOOL(v));
    break;
  case NANBOX_TUNDEF:
    SIDEBUG("undefined");
    break;
  case NANBOX_TNULL:
    SIDEBUG("null");
    break;
  case NANBOX_TIFN:
    SIDEBUG("internal function, type: %d, number: %d", NANBOX_IFN_TYPE(v), NANBOX_IFN_NUMBER(v));
    break;
  NANBOX_CASES_TPTR
    SIDEBUG("pointer to ");
    debug_heap_obj(SIHEAP_NANBOXTOPTR(v));
    break;
  default:
    if (NANBOX_ISFLOAT(v)) {
      SIDEBUG("float, value: %f", NANBOX_FLOAT(v));
    } else {
      SIDEBUG("unknown NaNbox value: %08x", v.as_i32);
    }
    break;
  }
}

const char *get_opcode_name(opcode_t op) {
  static const char *opcode_names[] = {
    "nop",
    "ldc_i",
    "lgc_i",
    "ldc_f32",
    "lgc_f32",
    "ldc_f64",
    "lgc_f64",
    "ldc_b_0",
    "ldc_b_1",
    "lgc_b_0",
    "lgc_b_1",
    "lgc_u",
    "lgc_n",
    "lgc_s",
    "pop_g",
    "pop_b",
    "pop_f",
    "add_g",
    "add_f",
    "sub_g",
    "sub_f",
    "mul_g",
    "mul_f",
    "div_g",
    "div_f",
    "mod_g",
    "mod_f",
    "not_g",
    "not_b",
    "lt_g",
    "lt_f",
    "gt_g",
    "gt_f",
    "le_g",
    "le_f",
    "ge_g",
    "ge_f",
    "eq_g",
    "eq_f",
    "eq_b",
    "new_c",
    "new_a",
    "ldl_g",
    "ldl_f",
    "ldl_b",
    "stl_g",
    "stl_b",
    "stl_f",
    "ldp_g",
    "ldp_f",
    "ldp_b",
    "stp_g",
    "stp_b",
    "stp_f",
    "lda_g",
    "lda_b",
    "lda_f",
    "sta_g",
    "sta_b",
    "sta_f",
    "br_t",
    "br_f",
    "br",
    "jmp",
    "call",
    "call_t",
    "call_p",
    "call_t_p",
    "call_v",
    "call_t_v",
    "ret_g",
    "ret_f",
    "ret_b",
    "ret_u",
    "ret_n",
    "dup",
    "newenv",
    "popenv",
    "new_c_p",
    "new_c_v",
    "neg_g",
    "neg_f",
    "neq_g",
    "neq_f",
    "neq_b"
  };

  if (op > op_neq_b) {
    return "invalid_opcode";
  } else {
    return opcode_names[op];
  }
}

void debug_heap_obj(siheap_header_t *o) {
  switch (o->type) {
  case sitype_env: {
    siheap_env_t *env = (siheap_env_t *) o;
    SIDEBUG("environment with %d entries; parent at %p", env->entry_count, (void *) env->parent);
    break;
  }
  case sitype_frame: {
    siheap_frame_t *fr = (siheap_frame_t *) o;
    SIDEBUG("frame; return address %tx, stack bottom %p, limit %p, top %p, saved environment %p", fr->return_address - sistate.program,
      (void *) fr->saved_stack_bottom, (void *) fr->saved_stack_limit, (void *) fr->saved_stack_top, (void *) fr->saved_env);
    break;
  }
  case sinter_type_function: {
    siheap_function_t *f = (siheap_function_t *) o;
    SIDEBUG("function; code address %tx, environment %p",
      (const opcode_t *) f->code - sistate.program, (void *) f->env);
    break;
  }
  }
}
#endif
