#ifndef SINTER_NANBOX_H
#define SINTER_NANBOX_H

#include <sinter.h>

#include "memory.h"

/*
 * Sinter 32-bit NaN-boxing
 *
 * In a 32-bit float, a NaN is any value which has the 8 exponent bits set and
 * a nonzero mantissa field. Normal float operations only produce the canonical
 * quiet NaN (qNaN) with only the MSB of the mantissa field set, that is,
 * 0x7FC00000.
 *
 * We'd like to have two contiguous ranges to represent small integers and heap
 * pointers in a way that allows us to retrieve the actual values in just one
 * operation.
 *
 * There are a few special values:
 * - canonical qNaN 0x7fc00000
 * - +infinity 0x7f800000
 * - -infinity 0xff800000
 * that we cannot use. The first two cause "holes" preventing us from using the
 * range 0x7f800000 to 0x7fffffff directly (a 23-bit range). The last one
 * prevents us from using the range 0xff800000 to 0xffffffff directly (another
 * 23-bit range).
 *
 * As a compromise, we use:
 * - 0xffc00000 to 0xffffffff for pointers (22 bits)
 * - 0x7fe00000 to 0x7fffffff for small integers (21 bits)
 *
 * The following ranges are unused:
 * - 0x7f800001 to 0x7fbfffff (22 bits minus 1)
 * - 0x7fc00001 to 0x7fdfffff (21 bits minus 1)
 * - 0xff800001 to 0xffbfffff (22 bits minus 1)
 * so we use them to represent the other singleton primitives.
 *
 * We represent 7 values/types:
 * - empty: 0x7f900000
 * - undefined: 0x7fa00000
 * - null: 0x7fb00000
 * - false: 0x7fd00000
 * - true: 0x7fd00001
 * - small integers: val & 0xffe00000 == 0x7fe00000
 * - heap pointers: val >= 0xffc00000
 *
 * Thus to check the type, we can simply check (val & 0xfff00000).
 */

typedef union sinanbox {
  float as_float;
  uint32_t as_i32;
} sinanbox_t;

#define NANBOX_TYPEMASK 0xfff00000u
#define NANBOX_TEMPTY 0x7f900000u
#define NANBOX_TUNDEF 0x7fa00000u
#define NANBOX_TNULL 0x7fb00000u
#define NANBOX_TBOOL 0x7fd00000u
#define NANBOX_TNUM 0x7fe00000u
#define NANBOX_TPTR 0xffc00000u
#define NANBOX_GETTYPE(val) ((val).as_i32 & NANBOX_TYPEMASK)

#define NANBOX_ISFLOAT(val) ((((val).as_i32 & 0x7f800000) != 0x7f800000) || ((val).as_i32 == 0x7fc00000))
#define NANBOX_ISEMPTY(val) (NANBOX_GETTYPE(val) == NANBOX_TEMPTY)
#define NANBOX_ISUNDEF(val) (NANBOX_GETTYPE(val) == NANBOX_TUNDEF)
#define NANBOX_ISNULL(val) (NANBOX_GETTYPE(val) == NANBOX_TNULL)
#define NANBOX_ISBOOL(val) (NANBOX_GETTYPE(val) == NANBOX_TBOOL)
#define NANBOX_ISNUM(val) ((val).as_i32 & NANBOX_TNUM == NANBOX_TNUM)
#define NANBOX_ISPTR(val) ((val).as_i32 & NANBOX_TPTR == NANBOX_TPTR)

#define NANBOX_FLOAT(val) ((val).as_float)
#define NANBOX_BOOL(val) ((val).as_i32 & 1)
#define NANBOX_NUMBER(val) (nanbox_number(val))
#define NANBOX_PTR(val) ((val).as_i32 & 0x3fffffu)

signed int nanbox_number(sinanbox_t val) {
  struct { signed int n : 21; } s;
  s.n = val.as_i32;
  return s.n;
}
#endif
