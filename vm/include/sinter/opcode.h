#ifndef SINTER_OPCODE_H
#define SINTER_OPCODE_H

#include "config.h"

#include <float.h>
#include <stdint.h>

typedef unsigned char opcode_t;
typedef uint32_t address_t;
typedef int32_t offset_t;

typedef enum __attribute__((__packed__)) {
  op_nop      = 0x00,
  op_ldc_i    = 0x01,
  op_lgc_i    = 0x02,
  op_ldc_f32  = 0x03,
  op_lgc_f32  = 0x04,
  op_ldc_f64  = 0x05,
  op_lgc_f64  = 0x06,
  op_ldc_b_0  = 0x07,
  op_ldc_b_1  = 0x08,
  op_lgc_b_0  = 0x09,
  op_lgc_b_1  = 0x0A,
  op_lgc_u    = 0x0B,
  op_lgc_n    = 0x0C,
  op_lgc_s    = 0x0D,
  op_pop_g    = 0x0E,
  op_pop_b    = 0x0F,
  op_pop_f    = 0x10,
  op_add_g    = 0x11,
  op_add_f    = 0x12,
  op_sub_g    = 0x13,
  op_sub_f    = 0x14,
  op_mul_g    = 0x15,
  op_mul_f    = 0x16,
  op_div_g    = 0x17,
  op_div_f    = 0x18,
  op_mod_g    = 0x19,
  op_mod_f    = 0x1A,
  op_not_g    = 0x1B,
  op_not_b    = 0x1C,
  op_lt_g     = 0x1D,
  op_lt_f     = 0x1E,
  op_gt_g     = 0x1F,
  op_gt_f     = 0x20,
  op_le_g     = 0x21,
  op_le_f     = 0x22,
  op_ge_g     = 0x23,
  op_ge_f     = 0x24,
  op_eq_g     = 0x25,
  op_eq_f     = 0x26,
  op_eq_b     = 0x27,
  op_new_c    = 0x28,
  op_new_a    = 0x29,
  op_ldl_g    = 0x2A,
  op_ldl_f    = 0x2B,
  op_ldl_b    = 0x2C,
  op_stl_g    = 0x2D,
  op_stl_b    = 0x2E,
  op_stl_f    = 0x2F,
  op_ldp_g    = 0x30,
  op_ldp_f    = 0x31,
  op_ldp_b    = 0x32,
  op_stp_g    = 0x33,
  op_stp_b    = 0x34,
  op_stp_f    = 0x35,
  op_lda_g    = 0x36,
  op_lda_b    = 0x37,
  op_lda_f    = 0x38,
  op_sta_g    = 0x39,
  op_sta_b    = 0x3A,
  op_sta_f    = 0x3B,
  op_br_t     = 0x3C,
  op_br_f     = 0x3D,
  op_br       = 0x3E,
  op_jmp      = 0x3F,
  op_call     = 0x40,
  op_call_t   = 0x41,
  op_call_p   = 0x42,
  op_call_t_p = 0x43,
  op_call_v   = 0x44,
  op_call_t_v = 0x45,
  op_ret_g    = 0x46,
  op_ret_f    = 0x47,
  op_ret_b    = 0x48,
  op_ret_u    = 0x49,
  op_ret_n    = 0x4A,
  op_dup      = 0x4B,
  op_newenv   = 0x4C,
  op_popenv   = 0x4D,
  op_new_c_p  = 0x4E,
  op_new_c_v  = 0x4F,
  op_neg_g    = 0x50,
  op_neg_f    = 0x51,
  op_neq_g    = 0x52,
  op_neq_f    = 0x53,
  op_neq_b    = 0x54
} sinter_opcode_t;
_Static_assert(sizeof(sinter_opcode_t) == 1, "enum sinter_opcode has wrong size");

#ifdef SINTER_OPSTRUCT
#error Conflicting SINTER_OPSTRUCT defined.
#endif
#define SINTER_OPSTRUCT(__ident__, __size__, __body__) \
  struct __attribute__((__packed__)) op_ ## __ident__ { \
    opcode_t opcode; \
    __body__ \
  }; \
  _Static_assert(sizeof(struct op_ ## __ident__) == __size__ + 1, "struct op_" #__ident__ " has wrong size");

SINTER_OPSTRUCT(i32, 4,
  int32_t operand;
)

SINTER_OPSTRUCT(f32, 4,
  float operand;
)

#if DBL_MANT_DIG == 53 && !defined(SINTER_TEST_SHORT_DOUBLE)
SINTER_OPSTRUCT(f64, 8,
  double operand;
)
_Static_assert(sizeof(double) == 8, "double is not 64-bit");
#else
#define SINTER_SHORT_DOUBLE_WORKAROUND
SINTER_OPSTRUCT(f64, 8,
  uint64_t operand_u64;
)
_Static_assert(sizeof(uint64_t) == 8, "uint64_t is not 64-bit");
_Static_assert(sizeof(float) == 4, "float is not 32-bit");
#endif

SINTER_OPSTRUCT(address, 4,
  address_t address;
)

SINTER_OPSTRUCT(oneindex, 1,
  uint8_t index;
)

SINTER_OPSTRUCT(twoindex, 2,
  uint8_t index;
  uint8_t envindex;
)

SINTER_OPSTRUCT(offset, 4,
  offset_t offset;
)

SINTER_OPSTRUCT(call, 1,
  uint8_t num_args;
)

SINTER_OPSTRUCT(call_internal, 2,
  uint8_t id;
  uint8_t num_args;
)

#undef SINTER_OPSTRUCT

#endif // SINTER_OPCODE_H
