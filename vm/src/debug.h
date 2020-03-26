#ifndef SINTER_DEBUG_H
#define SINTER_DEBUG_H

#include "opcode.h"
#include "nanbox.h"

#if SINTER_DEBUG_LEVEL >= 1
#include <stdio.h>
#define SIDEBUG(...) fprintf(stderr, __VA_ARGS__)

static inline const char *get_opcode_name(opcode_t op) {
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
#else
#define SIDEBUG(...) ((void) 0)
#define get_opcode_name(x) ""
#endif

#if SINTER_DEBUG_LEVEL >= 2
#include <stdio.h>
#define SITRACE(...) fprintf(stderr, __VA_ARGS__)
#else
#define SITRACE(...) ((void) 0)
#endif

#define SIBUG() SIDEBUG("BUG at %s at %s:%d\n", __func__, __FILE__, __LINE__);
#define SIBUGM(msg) SIDEBUG("BUG at %s at %s:%d: " msg, __func__, __FILE__, __LINE__);
#define SIBUGV(msg, ...) SIDEBUG("BUG at %s at %s:%d: " msg, __func__, __FILE__, __LINE__, __VA_ARGS__);

#endif
