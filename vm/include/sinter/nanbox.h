#ifndef SINTER_NANBOX_H
#define SINTER_NANBOX_H

#include <stdint.h>

/**
 * Sinter 32-bit NaN-box
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
 * - primitive function objects: 0xff9000xx
 * - vm-internal function objects: 0xff9001xx
 *
 * Thus to check the type, we can simply check (val & 0xfff00000).
 */
typedef union {
  float as_float;
  uint32_t as_i32;
} sinanbox_t;
_Static_assert(sizeof(sinanbox_t) == 4, "sinanbox_t has wrong size");

#define NANBOX_TYPEMASK 0xfff00000u
#define NANBOX_TEMPTY 0x7f900000u
#define NANBOX_TUNDEF 0x7fa00000u
#define NANBOX_TNULL 0x7fb00000u
#define NANBOX_TBOOL 0x7fd00000u
#define NANBOX_TINT 0x7fe00000u
#define NANBOX_TPTR 0xffc00000u
#define NANBOX_TIFN 0xff900000u

#define NANBOX_CASES_TINT case NANBOX_TINT: case 0x7ff00000u:
#define NANBOX_CASES_TPTR case NANBOX_TPTR: case 0xffd00000u: \
  case 0xffe00000u: case 0xfff00000u:
#define NANBOX_GETTYPE(val) ((val).as_i32 & NANBOX_TYPEMASK)

#define NANBOX_ISFLOAT(val) (nanbox_isfloat(val))
#define NANBOX_ISEMPTY(val) ((val).as_i32 == NANBOX_TEMPTY)
#define NANBOX_ISUNDEF(val) ((val).as_i32 == NANBOX_TUNDEF)
#define NANBOX_ISNULL(val) ((val).as_i32 == NANBOX_TNULL)
#define NANBOX_ISBOOL(val) (NANBOX_GETTYPE(val) == NANBOX_TBOOL)
#define NANBOX_ISINT(val) (((val).as_i32 & NANBOX_TINT) == NANBOX_TINT)
#define NANBOX_ISPTR(val) (((val).as_i32 & NANBOX_TPTR) == NANBOX_TPTR)
#define NANBOX_ISIFN(val) (NANBOX_GETTYPE(val) == NANBOX_TIFN)
#define NANBOX_ISNUMERIC(val) (NANBOX_ISFLOAT(val) || NANBOX_ISINT(val))

#define NANBOX_FLOAT(val) ((val).as_float)
#define NANBOX_BOOL(val) ((val).as_i32 & 1)
#define NANBOX_INT(val) (((struct { signed int n : 21; }) { .n = (val).as_i32 }).n)
#define NANBOX_PTR(val) ((val).as_i32 & 0x3fffffu)

#define NANBOX_IFN_TYPE(val) (((val).as_i32 & 0x100) >> 8)
#define NANBOX_IFN_NUMBER(val) ((val).as_i32 & 0xff)

#define NANBOX_OFFLOAT(val) ((sinanbox_t) { .as_float = (val) })
#define NANBOX_OFEMPTY() ((sinanbox_t) { .as_i32 = NANBOX_TEMPTY })
#define NANBOX_OFUNDEF() ((sinanbox_t) { .as_i32 = NANBOX_TUNDEF })
#define NANBOX_OFNULL() ((sinanbox_t) { .as_i32 = NANBOX_TNULL })
#define NANBOX_OFBOOL(val) ((sinanbox_t) { .as_i32 = (!!(val)) | NANBOX_TBOOL })
#define NANBOX_OFINT(val) ((sinanbox_t) { .as_i32 = ((val) & 0x1fffffu) | NANBOX_TINT })
#define NANBOX_OFPTR(val) ((sinanbox_t) { .as_i32 = ((val) & 0x3fffffu) | NANBOX_TPTR })

#define NANBOX_OFIFN_PRIMITIVE(number) ((sinanbox_t) { .as_i32 = NANBOX_TIFN | number })
#define NANBOX_OFIFN_VM(number) ((sinanbox_t) { .as_i32 = NANBOX_TIFN | 0x100 | number })

#define NANBOX_INTMAX 0xFFFFF
#define NANBOX_INTMIN (-0x100000)

#define NANBOX_CANONICAL_NAN ((sinanbox_t) { .as_i32 = 0x7FC00000u })
#define NANBOX_IDENTICAL(v0, v1) ((v0).as_i32 == (v1).as_i32)

static inline _Bool nanbox_isfloat(sinanbox_t v) {
  uint32_t i = v.as_i32;
  return ((i & 0x7f800000) != 0x7f800000) || ((i & 0x7fffff) == 0) || i == 0x7fc00000;
}
#endif
