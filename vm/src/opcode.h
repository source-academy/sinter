#ifndef SINTER_OPCODE_H
#define SINTER_OPCODE_H

#include <stdint.h>

typedef unsigned char opcode_t;
typedef uint32_t address_t;
typedef uint32_t offset_t;

enum __attribute__((__packed__)) sinter_opcode {
  op_nop      = 0x00,
  op_ldc_i    = 0x01,
  op_ldc_f    = 0x02,
  op_ldc_b0   = 0x03,
  op_ldc_b1   = 0x04,
  op_ldc_u    = 0x05,
  op_ldc_n    = 0x06,
  op_ldc_s    = 0x07,
  op_pop      = 0x08,
  op_add      = 0x09,
  op_sub      = 0x0A,
  op_mul      = 0x0B,
  op_div      = 0x0C,
  op_mod      = 0x0D,
  op_not      = 0x0E,
  op_lt       = 0x0F,
  op_gt       = 0x10,
  op_le       = 0x11,
  op_ge       = 0x12,
  op_eq       = 0x13,
  op_new_f    = 0x14,
  op_new_v    = 0x15,
  op_ld_e     = 0x16,
  op_st_e     = 0x17,
  op_ld_ep    = 0x18,
  op_st_ep    = 0x19,
  op_ld_a     = 0x1A,
  op_st_a     = 0x1B,
  op_ld_ap    = 0x1C,
  op_st_ap    = 0x1D,
  op_ld_v     = 0x1E,
  op_st_v     = 0x1F,
  op_br_t     = 0x20,
  op_br       = 0x21,
  op_jmp      = 0x22,
  op_call     = 0x23,
  op_call_t   = 0x24,
  op_call_p   = 0x25,
  op_call_tp  = 0x26,
  op_call_v   = 0x27,
  op_call_tv  = 0x28,
  op_ret      = 0x29,
  op_ret_u    = 0x2A
};
_Static_assert(sizeof(enum sinter_opcode) == 1, "enum sinter_opcode has wrong size");

#ifdef SINTER_OPSTRUCT
#error Conflicting SINTER_OPSTRUCT defined.
#endif
#define SINTER_OPSTRUCT(__ident__, __size__, __body__) \
  struct __attribute__((__packed__)) op_ ## __ident__ { \
    opcode_t opcode; \
    __body__ \
  }; \
  _Static_assert(sizeof(struct op_ ## __ident__) == __size__ + 1, "struct op_" #__ident__ " has wrong size");

SINTER_OPSTRUCT(ldc_i, 4,
  uint32_t operand;
)

SINTER_OPSTRUCT(ldc_f, 4,
  float operand;
)

SINTER_OPSTRUCT(ldc_s, 4,
  address_t address;
)

SINTER_OPSTRUCT(new_f, 6,
  uint8_t maxstack;
  uint8_t framesize;
  address_t code;
)

SINTER_OPSTRUCT(ld_e, 1,
  uint8_t index;
)

SINTER_OPSTRUCT(st_e, 1,
  uint8_t index;
)

SINTER_OPSTRUCT(ld_ep, 2,
  uint8_t index;
  uint8_t envindex;
)

SINTER_OPSTRUCT(st_ep, 2,
  uint8_t index;
  uint8_t envindex;
)

SINTER_OPSTRUCT(ld_a, 1,
  uint8_t index;
)

SINTER_OPSTRUCT(st_a, 1,
  uint8_t index;
)

SINTER_OPSTRUCT(ld_ap, 2,
  uint8_t index;
  uint8_t envindex;
)

SINTER_OPSTRUCT(st_ap, 2,
  uint8_t index;
  uint8_t envindex;
)

SINTER_OPSTRUCT(br_t, 4,
  offset_t offset;
)

SINTER_OPSTRUCT(br, 4,
  offset_t offset;
)

SINTER_OPSTRUCT(jmp, 4,
  address_t address;
)

SINTER_OPSTRUCT(call, 1,
  uint8_t numargs;
)

SINTER_OPSTRUCT(call_t, 1,
  uint8_t numargs;
)

SINTER_OPSTRUCT(call_p, 2,
  uint8_t id;
  uint8_t numargs;
)

SINTER_OPSTRUCT(call_tp, 2,
  uint8_t id;
  uint8_t numargs;
)

SINTER_OPSTRUCT(call_v, 2,
  uint8_t id;
  uint8_t numargs;
)

SINTER_OPSTRUCT(call_tv, 2,
  uint8_t id;
  uint8_t numargs;
)

#undef SINTER_OPSTRUCT

#endif // SINTER_OPCODE_H
