#include <sinter/config.h>

#include <stdio.h>
#include <inttypes.h>

#include <sinter/nanbox.h>
#include <sinter/heap.h>
#include <sinter/vm.h>
#include <sinter/debug.h>
#include <sinter/debug_heap.h>
#include <sinter/heap_obj.h>

#if SINTER_DEBUG_LOGLEVEL >= 1
void debug_nanbox(sinanbox_t v) {
  SIDEBUG("%"PRIx32 " ", v.as_u32);
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
    SIDEBUG("internal function, type: %s, number: %d", NANBOX_IFN_TYPE(v) ? "VM-internal" : "primitive", NANBOX_IFN_NUMBER(v));
    break;
  NANBOX_CASES_TPTR
    SIDEBUG("pointer to ");
    SIDEBUG_HEAPOBJ(SIHEAP_NANBOXTOPTR(v));
    break;
  default:
    if (NANBOX_ISFLOAT(v)) {
      SIDEBUG("float, value: %f", NANBOX_FLOAT(v));
    } else {
      SIDEBUG("unknown NaNbox value: %08x", v.as_u32);
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

void debug_heap_obj(const siheap_header_t *o) {
  SIDEBUG("(address %p) ", (void *) o);
  switch (o->type) {
  case sitype_env: {
    const siheap_env_t *env = (const siheap_env_t *) o;
    SIDEBUG("environment with %d entries; parent at %p", env->entry_count, (void *) env->parent);
    break;
  }
  case sitype_frame: {
    const siheap_frame_t *fr = (const siheap_frame_t *) o;
    SIDEBUG("frame; return address %tx, stack bottom %p, limit %p, top %p, saved environment %p",
      fr->return_address ? (fr->return_address - sistate.program) : 0,
      (void *) fr->saved_stack_bottom, (void *) fr->saved_stack_limit, (void *) fr->saved_stack_top, (void *) fr->saved_env);
    break;
  }
  case sitype_function: {
    const siheap_function_t *f = (const siheap_function_t *) o;
    SIDEBUG("function; code address %tx, environment %p",
      (const opcode_t *) f->code - sistate.program, (void *) f->env);
    break;
  }
  case sitype_strpair: {
    const siheap_strpair_t *s = (const siheap_strpair_t *) o;
    SIDEBUG("string pair; left %p, right %p", (void *) s->left, (void *) s->right);
    break;
  }
  case sitype_strconst: {
    const siheap_strconst_t *s = (const siheap_strconst_t *) o;
    SIDEBUG("string constant; address %tx; value \"%s\"", (const opcode_t *) s->string - sistate.program, s->string->data);
    break;
  }
  case sitype_string: {
    const siheap_string_t *s = (const siheap_string_t *) o;
    SIDEBUG("string; address %p; value \"%s\"", (void *) s, s->string);
    break;
  }
  case sitype_array: {
    const siheap_array_t *a = (const siheap_array_t *) o;
    SIDEBUG("array; address %p; data address %p; count %d; allocated %d", (void *) a, (void *) a->data, a->count, a->alloc_size);
    break;
  }
  case sitype_intcont: {
    const siheap_intcont_t *c = (const siheap_intcont_t *) o;
    SIDEBUG("function (internal continuation); argc %d", c->argc);
    break;
  }
  case sitype_array_data:
  case sitype_empty:
  case sitype_free:
  default:
    SIDEBUG("unknown heap object type %d at address %p", o->type, (void *) o);
    break;
  }
}
#endif
