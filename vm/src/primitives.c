#include <sinter/config.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include <sinter/nanbox.h>
#include <sinter/fault.h>
#include <sinter/heap.h>
#include <sinter/stack.h>
#include <sinter/debug.h>
#include <sinter/internal_fn.h>
#include <sinter/vm.h>
#include <sinter/display.h>

/**
 * This file contains the implementations (in C) of all 92 functions in the Source
 * standard library.
 */

static void unimpl(void) {
  SIBUGV("Unimplemented primitive function %02x at address 0x%tx\n", *(sistate.pc + 1), SISTATE_CURADDR);
  sifault(sinter_fault_invalid_program);
}

static void debug_display_argv(unsigned int argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  for (unsigned int i = 0; i < argc; ++i) {
    SIDEBUG("%d: ", i);
    SIDEBUG_NANBOX(argv[i]);
    SIDEBUG("\n");
  }
}

static void handle_display(unsigned int argc, sinanbox_t *argv, bool is_error) {
  if (argc < 1) {
    return;
  }

  if (argc > 1) {
    sidisplay_nanbox(argv[1], is_error);
    SIVMFN_PRINT(" ", is_error);
  }

  sidisplay_nanbox(argv[0], is_error);
  SIVMFN_PRINT("\n", is_error);
}

#define CHECK_ARGC(n) do { \
  if (argc < (n)) { \
    sifault(sinter_fault_function_arity); \
    return NANBOX_OFEMPTY(); \
  } \
} while (0)

/******************************************************************************
 * Basic type-checking primitives
 ******************************************************************************/

static sinanbox_t sivmfn_prim_is_array(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  sinanbox_t v = *argv;
  return NANBOX_OFBOOL(NANBOX_ISPTR(v) && ((siheap_header_t *) SIHEAP_NANBOXTOPTR(v))->type == sitype_array);
}

static sinanbox_t sivmfn_prim_is_boolean(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  return NANBOX_OFBOOL(NANBOX_ISBOOL(*argv));
}

static sinanbox_t sivmfn_prim_is_function(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  sinanbox_t v = *argv;

  if (NANBOX_ISPTR(v)) {
    siheap_type_t type = ((siheap_header_t *) SIHEAP_NANBOXTOPTR(v))->type;
    return NANBOX_OFBOOL(type == sitype_function || type == sitype_intcont);
  }

  return NANBOX_OFBOOL(NANBOX_ISIFN(v));
}

static sinanbox_t sivmfn_prim_is_null(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  return NANBOX_OFBOOL(NANBOX_ISNULL(*argv));
}

static sinanbox_t sivmfn_prim_is_number(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  return NANBOX_OFBOOL(NANBOX_ISNUMERIC(*argv));
}

static sinanbox_t sivmfn_prim_is_stream(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_is_string(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  sinanbox_t v = *argv;
  return NANBOX_OFBOOL(NANBOX_ISPTR(v) && siheap_is_string(SIHEAP_NANBOXTOPTR(v)));
}

static sinanbox_t sivmfn_prim_is_undefined(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  return NANBOX_OFBOOL(NANBOX_ISUNDEF(*argv));
}

/******************************************************************************
 * Math primitives
 ******************************************************************************/

#define MATH_FN(name) static sinanbox_t sivmfn_prim_math_ ## name(uint8_t argc, sinanbox_t *argv) { \
  CHECK_ARGC(1); \
  return NANBOX_OFFLOAT(name ## f(NANBOX_TOFLOAT(*argv))); \
}

#define MATH_FN_2(name) static sinanbox_t sivmfn_prim_math_ ## name(uint8_t argc, sinanbox_t *argv) { \
  CHECK_ARGC(2); \
  return NANBOX_OFFLOAT(name ## f(NANBOX_TOFLOAT(argv[0]), NANBOX_TOFLOAT(argv[1]))); \
}

MATH_FN(acos)
MATH_FN(acosh)
MATH_FN(asin)
MATH_FN(asinh)
MATH_FN(atan)
MATH_FN(atanh)
MATH_FN(cbrt)
MATH_FN(cos)
MATH_FN(cosh)
MATH_FN(exp)
MATH_FN(expm1)
MATH_FN(log)
MATH_FN(log1p)
MATH_FN(log2)
MATH_FN(log10)
MATH_FN(sin)
MATH_FN(sinh)
MATH_FN(sqrt)
MATH_FN(tan)
MATH_FN(tanh)
MATH_FN_2(atan2)
MATH_FN_2(pow)

static sinanbox_t sivmfn_prim_math_hypot(uint8_t argc, sinanbox_t *argv) {
  // Adapted from https://github.com/v8/v8/blob/master/src/builtins/math.tq#L405
  if (argc == 0) {
    return NANBOX_OFINT(0);
  }

  float max = 0;
  for (unsigned int i = 0; i < argc; ++i) {
    if (NANBOX_IDENTICAL(argv[i], NANBOX_CANONICAL_NAN)) {
      return NANBOX_CANONICAL_NAN;
    }
    float v = NANBOX_TOFLOAT(argv[i]);
    if (v > max) {
      max = v;
    }
  }

  if (max == 0.0f) {
    return NANBOX_OFINT(0);
  }

  float sum = 0;
  float compensation = 0;
  for (unsigned int i = 0; i < argc; ++i) {
    float n = NANBOX_TOFLOAT(argv[i]) / max;
    float summand = n * n - compensation;
    float preliminary = sum + summand;
    compensation = (preliminary - sum) - summand;
    sum = preliminary;
  }

  return NANBOX_OFFLOAT(sqrtf(sum) * max);
}

static sinanbox_t sivmfn_prim_math_abs(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  sinanbox_t v = *argv;

  if (NANBOX_ISINT(v)) {
    return NANBOX_WRAP_INT(abs(NANBOX_INT(v)));
  } else if (NANBOX_ISFLOAT(v)) {
    return NANBOX_OFFLOAT(fabsf(NANBOX_FLOAT(v)));
  }

  sifault(sinter_fault_type);
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_clz32(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  uint32_t clzarg = NANBOX_TOU32(*argv);

  if (clzarg == 0) {
    return NANBOX_OFINT(32);
  }

  unsigned int ret;
#if UINT_MAX == UINT32_MAX
  ret = __builtin_clz(clzarg);
#elif ULONG_MAX == UINT32_MAX
  ret = __builtin_clzl(clzarg);
#elif ULLONG_MAX == UINT32_MAX
  ret = __builtin_clzll(clzarg);
#endif
  return NANBOX_OFINT(ret);
}

static sinanbox_t sivmfn_prim_math_fround(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  // no-op: sinter uses floats not doubles
  return *argv;
}

static sinanbox_t sivmfn_prim_math_imul(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  uint32_t r = NANBOX_TOU32(argv[0])*NANBOX_TOU32(argv[1]);
  if (r > INT32_MAX) {
    r -= 0x80000000ull;
  }
  return NANBOX_WRAP_INT((int32_t) r);
}

static sinanbox_t sivmfn_prim_math_max(uint8_t argc, sinanbox_t *argv) {
  if (argc == 0) {
    return NANBOX_OFFLOAT(-INFINITY);
  }

  sinanbox_t max = argv[0];
  if (NANBOX_IDENTICAL(max, NANBOX_CANONICAL_NAN)) {
    return max;
  }
  if (!NANBOX_ISFLOAT(max) && !NANBOX_ISINT(max)) {
    sifault(sinter_fault_type);
    return max;
  }
  for (unsigned int i = 1; i < argc; ++i) {
    sinanbox_t contender = argv[i];
    if (NANBOX_IDENTICAL(contender, NANBOX_CANONICAL_NAN)) {
      return contender;
    }
    if (NANBOX_ISINT(max) && NANBOX_ISINT(contender)) {
      int32_t maxi = NANBOX_INT(max);
      int32_t contenderi = NANBOX_INT(contender);
      if (contenderi > maxi) {
        max = contender;
      }
    } else if (NANBOX_ISFLOAT(contender) || NANBOX_ISINT(contender)) {
      float maxf = NANBOX_TOFLOAT(max);
      float contenderf = NANBOX_TOFLOAT(contender);
      if (contenderf > maxf) {
        max = contender;
      }
    } else {
      sifault(sinter_fault_type);
    }
  }

  return max;
}

static sinanbox_t sivmfn_prim_math_min(uint8_t argc, sinanbox_t *argv) {
  if (argc == 0) {
    return NANBOX_OFFLOAT(INFINITY);
  }

  sinanbox_t min = argv[0];
  if (NANBOX_IDENTICAL(min, NANBOX_CANONICAL_NAN)) {
    return min;
  }
  if (!NANBOX_ISFLOAT(min) && !NANBOX_ISINT(min)) {
    sifault(sinter_fault_type);
    return min;
  }
  for (unsigned int i = 1; i < argc; ++i) {
    sinanbox_t contender = argv[i];
    if (NANBOX_IDENTICAL(contender, NANBOX_CANONICAL_NAN)) {
      return contender;
    }
    if (NANBOX_ISINT(min) && NANBOX_ISINT(contender)) {
      int32_t mini = NANBOX_INT(min);
      int32_t contenderi = NANBOX_INT(contender);
      if (contenderi < mini) {
        min = contender;
      }
    } else if (NANBOX_ISFLOAT(contender) || NANBOX_ISINT(contender)) {
      float minf = NANBOX_TOFLOAT(min);
      float contenderf = NANBOX_TOFLOAT(contender);
      if (contenderf < minf) {
        min = contender;
      }
    } else {
      sifault(sinter_fault_type);
    }
  }

  return min;
}
static sinanbox_t sivmfn_prim_math_random(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  return NANBOX_OFFLOAT((float) rand() / (((float) RAND_MAX) + 1.0f));
}

static sinanbox_t sivmfn_prim_math_sign(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  sinanbox_t v = *argv;
  if (NANBOX_ISINT(v)) {
    int32_t intv = NANBOX_INT(v);
    return NANBOX_OFINT(intv ? (intv > 0 ? 1 : -1) : 0);
  } else if (NANBOX_ISFLOAT(v)) {
    float fv = NANBOX_FLOAT(v);
    return NANBOX_OFINT(fv != 0.0f ? (fv > 0.0f ? 1 : -1) : 0);
  } else {
    sifault(sinter_fault_type);
  }
  return NANBOX_OFEMPTY();
}

#define FLOAT_ROUND_FN(name) static sinanbox_t sivmfn_prim_math_## name(uint8_t argc, sinanbox_t *argv) { \
  CHECK_ARGC(1); \
  sinanbox_t v = *argv; \
  if (NANBOX_ISINT(v)) { \
    return v; \
  } else if (NANBOX_ISFLOAT(v)) { \
    float retv = name ## f(NANBOX_FLOAT(v)); \
    if (retv >= NANBOX_INTMIN && retv <= NANBOX_INTMAX) { \
      return NANBOX_OFINT((int32_t) retv); \
    } \
    return NANBOX_OFFLOAT(retv); \
  } else { \
    sifault(sinter_fault_type); \
  } \
  return NANBOX_OFEMPTY(); \
}

FLOAT_ROUND_FN(floor)
FLOAT_ROUND_FN(ceil)
FLOAT_ROUND_FN(round)
FLOAT_ROUND_FN(trunc)

/******************************************************************************
 * Pair primitives
 ******************************************************************************/

static inline siheap_array_t *source_pair_ptr(sinanbox_t l, sinanbox_t r) {
  siheap_array_t *arr = siarray_new(2);
  siarray_put(arr, 0, l);
  siarray_put(arr, 1, r);
  return arr;
}

static inline sinanbox_t source_pair(sinanbox_t l, sinanbox_t r) {
  return SIHEAP_PTRTONANBOX(source_pair_ptr(l, r));
}

static inline siheap_array_t *nanbox_toarray(sinanbox_t p) {
  siheap_header_t *v = SIHEAP_NANBOXTOPTR(p);
  siheap_array_t *a = (siheap_array_t *) v;
  if (!NANBOX_ISPTR(p) || v->type != sitype_array) {
    sifault(sinter_fault_type);
    return NULL;
  }
  return a;
}

static inline sinanbox_t source_head(sinanbox_t p) {
  siheap_array_t *a = nanbox_toarray(p);
  return siarray_get(a, 0);
}

static inline sinanbox_t source_tail(sinanbox_t p) {
  siheap_array_t *a = nanbox_toarray(p);
  return siarray_get(a, 1);
}

static sinanbox_t sivmfn_prim_pair(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  siheap_refbox(argv[0]);
  siheap_refbox(argv[1]);
  return source_pair(argv[0], argv[1]);
}

static sinanbox_t sivmfn_prim_head(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  sinanbox_t ret = source_head(argv[0]);
  siheap_refbox(ret);
  return ret;
}

static sinanbox_t sivmfn_prim_tail(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  sinanbox_t ret = source_tail(argv[0]);
  siheap_refbox(ret);
  return ret;
}

static sinanbox_t sivmfn_prim_set_head(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  siheap_array_t *a = nanbox_toarray(argv[0]);
  siheap_refbox(argv[1]);
  siarray_put(a, 0, argv[1]);
  return NANBOX_OFUNDEF();
}

static sinanbox_t sivmfn_prim_set_tail(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  siheap_array_t *a = nanbox_toarray(argv[0]);
  siheap_refbox(argv[1]);
  siarray_put(a, 1, argv[1]);
  return NANBOX_OFUNDEF();
}

static sinanbox_t sivmfn_prim_is_pair(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  siheap_header_t *v = SIHEAP_NANBOXTOPTR(argv[0]);
  siheap_array_t *a = (siheap_array_t *) v;
  return NANBOX_OFBOOL(NANBOX_ISPTR(argv[0]) && v->type == sitype_array && a->count == 2);
}

/******************************************************************************
 * List primitives
 ******************************************************************************/

static sinanbox_t sivmfn_prim_is_list(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);

  sinanbox_t l = argv[0];
  while (!NANBOX_ISNULL(l)) {
    siheap_header_t *v = SIHEAP_NANBOXTOPTR(l);
    siheap_array_t *a = (siheap_array_t *) v;
    if (!NANBOX_ISPTR(l) || v->type != sitype_array || a->count != 2) {
      return NANBOX_OFBOOL(false);
    }
    l = siarray_get(a, 1);
  }

  return NANBOX_OFBOOL(true);
}

static inline size_t source_list_length(sinanbox_t l) {
  size_t length = 0;
  while (!NANBOX_ISNULL(l)) {
    ++length;
    l = source_tail(l);
  }
  return length;
}

static sinanbox_t sivmfn_prim_accumulate(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(3);

  // this function is particularly horrible for us
  // we don't want to have a naive recursive implementation (in case we blow the C stack
  // the Sinter stack isn't particularly large either
  // so.. here's a compromise: we allocate an array and flatten the list into it
  // then we accumulate using the array

  sinanbox_t f = argv[0], acc = argv[1];
  const size_t list_length = source_list_length(argv[2]);
  siheap_array_t *flat_list = siarray_new(list_length);
  siheap_intref(flat_list);
  // flatten the list into the array
  {
    size_t idx = 0;
    sinanbox_t l = argv[2];
    while (!NANBOX_ISNULL(l)) {
      siheap_array_t *pair = nanbox_toarray(l);
      sinanbox_t head = siarray_get(pair, 0);
      siheap_refbox(head);
      siarray_put(flat_list, idx, head);
      l = siarray_get(pair, 1);
      idx += 1;
    }
    assert(idx == list_length);
  }

  // the initial acc has a ref from the caller's stack (it's not removed until
  // after we return. if we are passing it to a reentrant call, then there will
  // be a second ref from our callee's env
  // ref it
  siheap_refbox(acc);
  for (size_t idxp1 = list_length; idxp1 > 0; --idxp1) {
    sinanbox_t f_args[] = {siarray_get(flat_list, idxp1 - 1), acc};
    siheap_refbox(f_args[0]);
    acc = siexec_nanbox(f, 2, f_args);
  }

  siheap_intderef(flat_list);
  siheap_deref(flat_list);

  return acc;
}

static sinanbox_t sivmfn_prim_append(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);

  sinanbox_t list = argv[0];

  if (NANBOX_ISNULL(list)) {
    siheap_refbox(argv[1]);
    return argv[1];
  }

  siheap_array_t *new_list = NULL;
  siheap_array_t *prev_pair = NULL;
  while (!NANBOX_ISNULL(list)) {
    siheap_array_t *pair = nanbox_toarray(list);
    sinanbox_t head = siarray_get(pair, 0);
    list = siarray_get(pair, 1);
    siheap_refbox(head);
    siheap_array_t *new_pair = source_pair_ptr(head, NANBOX_OFNULL());
    if (prev_pair) {
      siarray_put(prev_pair, 1, SIHEAP_PTRTONANBOX(new_pair));
    }
    if (!new_list) {
      new_list = new_pair;
    }
    prev_pair = new_pair;
  }

  siheap_refbox(argv[1]);
  siarray_put(prev_pair, 1, argv[1]);
  return SIHEAP_PTRTONANBOX(new_list);
}

static sinanbox_t sivmfn_prim_build_list(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);

  int32_t limit = NANBOX_TOI32(argv[0]);

  if (limit <= 0) {
    return NANBOX_OFNULL();
  }

  siheap_array_t *new_list = NULL;
  siheap_array_t *prev_pair = NULL;

  for (int32_t i = 0; i < limit; ++i) {
    sinanbox_t arg = NANBOX_WRAP_INT(i);
    sinanbox_t new_val = siexec_nanbox(argv[1], 1, &arg);
    siheap_array_t *new_pair = source_pair_ptr(new_val, NANBOX_OFNULL());
    if (prev_pair) {
      siarray_put(prev_pair, 1, SIHEAP_PTRTONANBOX(new_pair));
    }
    if (!new_list) {
      new_list = new_pair;
      siheap_intref(new_list);
    }
    prev_pair = new_pair;
  }

  siheap_intderef(new_list);
  return SIHEAP_PTRTONANBOX(new_list);
}

#define PRIM_ENUM_LIST_FN(type, each) static inline sinanbox_t enum_list_##type(type start, type end) { \
  if (start > end) { \
    return NANBOX_OFNULL(); \
  } \
  siheap_array_t *new_list = NULL; \
  siheap_array_t *prev_pair = NULL; \
  for (type i = start; i <= end; i += 1) { \
    siheap_array_t *new_pair = source_pair_ptr(each, NANBOX_OFNULL()); \
    if (prev_pair) { \
      siarray_put(prev_pair, 1, SIHEAP_PTRTONANBOX(new_pair)); \
    } \
    if (!new_list) { \
      new_list = new_pair; \
    } \
    prev_pair = new_pair; \
  } \
  return SIHEAP_PTRTONANBOX(new_list); \
}

PRIM_ENUM_LIST_FN(int32_t, NANBOX_WRAP_INT(i))
PRIM_ENUM_LIST_FN(float, NANBOX_OFFLOAT(i))

static sinanbox_t sivmfn_prim_enum_list(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);

  if (NANBOX_ISINT(argv[0])) {
      if (NANBOX_ISINT(argv[1])) {
        return enum_list_int32_t(NANBOX_INT(argv[0]), NANBOX_INT(argv[1]));
      } else if (NANBOX_ISFLOAT(argv[1])) {
        float end = NANBOX_FLOAT(argv[1]);
        if (end > INT32_MIN && end < INT32_MAX) {
          return enum_list_int32_t(NANBOX_TOI32(argv[0]), (int32_t) end);
        } else {
          return enum_list_float(NANBOX_INT(argv[0]), end);
        }
      }
  } else if (NANBOX_ISFLOAT(argv[0])) {
    return enum_list_float(NANBOX_FLOAT(argv[0]), NANBOX_TOFLOAT(argv[1]));
  }
  sifault(sinter_fault_type);
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_filter(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  if (NANBOX_ISNULL(argv[1])) {
    return NANBOX_OFNULL();
  }

  const sinanbox_t filter_fn = argv[0];

  sinanbox_t old_list = argv[1];
  siheap_array_t *new_list = NULL;
  siheap_array_t *prev_pair = NULL;
  while (!NANBOX_ISNULL(old_list)) {
    siheap_array_t *pair = nanbox_toarray(old_list);
    sinanbox_t cur = siarray_get(pair, 0);
    old_list = siarray_get(pair, 1);
    siheap_refbox(cur);
    sinanbox_t pred_result = siexec_nanbox(filter_fn, 1, &cur);
    if (NANBOX_ISBOOL(pred_result)) {
      if (!NANBOX_BOOL(pred_result)) {
        continue;
      }
    } else {
      sifault(sinter_fault_type);
    }
    siheap_refbox(cur);
    siheap_array_t *new_pair = source_pair_ptr(cur, NANBOX_OFNULL());
    if (prev_pair) {
      siarray_put(prev_pair, 1, SIHEAP_PTRTONANBOX(new_pair));
    }
    if (!new_list) {
      new_list = new_pair;
      siheap_intref(new_list);
    }
    prev_pair = new_pair;
  }

  if (new_list) {
    siheap_intderef(new_list);
    return SIHEAP_PTRTONANBOX(new_list);
  }

  return NANBOX_OFNULL();
}

static sinanbox_t sivmfn_prim_for_each(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  if (NANBOX_ISNULL(argv[1])) {
    return NANBOX_OFNULL();
  }

  const sinanbox_t for_each_fn = argv[0];

  sinanbox_t list = argv[1];
  while (!NANBOX_ISNULL(list)) {
    siheap_array_t *pair = nanbox_toarray(list);
    sinanbox_t cur = siarray_get(pair, 0);
    list = siarray_get(pair, 1);
    siheap_refbox(cur);
    siheap_derefbox(siexec_nanbox(for_each_fn, 1, &cur));
  }

  return NANBOX_OFUNDEF();
}

static sinanbox_t sivmfn_prim_length(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  return NANBOX_WRAP_UINT(source_list_length(argv[0]));
}

static sinanbox_t sivmfn_prim_list(uint8_t argc, sinanbox_t *argv) {
  if (argc == 0) {
    return NANBOX_OFNULL();
  }

  siheap_array_t *new_list = NULL;
  siheap_array_t *prev_pair = NULL;

  for (size_t i = 0; i < argc; ++i) {
    siheap_refbox(argv[i]);
    siheap_array_t *new_pair = source_pair_ptr(argv[i], NANBOX_OFNULL());
    if (prev_pair) {
      siarray_put(prev_pair, 1, SIHEAP_PTRTONANBOX(new_pair));
    }
    if (!new_list) {
      new_list = new_pair;
    }
    prev_pair = new_pair;
  }

  return SIHEAP_PTRTONANBOX(new_list);
}

static sinanbox_t sivmfn_prim_list_ref(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);

  sinanbox_t list = argv[0];
  int32_t count = NANBOX_TOI32(argv[1]);
  for (int32_t i = 0; i < count; ++i) {
    list = source_tail(list);
  }

  sinanbox_t retv = source_head(list);
  siheap_refbox(retv);
  return retv;
}

static sinanbox_t sivmfn_prim_list_to_string(uint8_t argc, sinanbox_t *argv) {
  // TODO: do we want to implement this?
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_map(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  if (NANBOX_ISNULL(argv[1])) {
    return NANBOX_OFNULL();
  }

  const sinanbox_t map_fn = argv[0];

  sinanbox_t old_list = argv[1];
  siheap_array_t *new_list = NULL;
  siheap_array_t *prev_pair = NULL;
  while (!NANBOX_ISNULL(old_list)) {
    siheap_array_t *pair = nanbox_toarray(old_list);
    sinanbox_t cur = siarray_get(pair, 0);
    old_list = siarray_get(pair, 1);
    siheap_refbox(cur);
    sinanbox_t newval = siexec_nanbox(map_fn, 1, &cur);
    siheap_array_t *new_pair = source_pair_ptr(newval, NANBOX_OFNULL());
    if (prev_pair) {
      siarray_put(prev_pair, 1, SIHEAP_PTRTONANBOX(new_pair));
    }
    if (!new_list) {
      new_list = new_pair;
      siheap_intref(new_list);
    }
    prev_pair = new_pair;
  }

  siheap_intderef(new_list);
  return SIHEAP_PTRTONANBOX(new_list);
}

static sinanbox_t sivmfn_prim_member(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  if (NANBOX_ISNULL(argv[1])) {
    return NANBOX_OFNULL();
  }

  const sinanbox_t needle = argv[0];

  sinanbox_t list = argv[1];
  while (!NANBOX_ISNULL(list)) {
    siheap_array_t *pair = nanbox_toarray(list);
    sinanbox_t cur = siarray_get(pair, 0);
    if (sivm_equal(needle, cur)) {
      break;
    }
    list = siarray_get(pair, 1);
  }

  siheap_refbox(list);
  return list;
}

static sinanbox_t sivmfn_prim_remove(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  if (NANBOX_ISNULL(argv[1])) {
    return NANBOX_OFNULL();
  }

  const sinanbox_t needle = argv[0];

  sinanbox_t list = argv[1];
  siheap_array_t *new_list = NULL;
  siheap_array_t *prev_pair = NULL;
  while (!NANBOX_ISNULL(list)) {
    siheap_array_t *pair = nanbox_toarray(list);
    sinanbox_t cur = siarray_get(pair, 0);
    list = siarray_get(pair, 1);

    if (sivm_equal(cur, needle)) {
      siheap_refbox(list);
      if (prev_pair) {
        assert(new_list);
        siarray_put(prev_pair, 1, list);
        return SIHEAP_PTRTONANBOX(new_list);
      } else {
        return list;
      }
    }

    siheap_refbox(cur);
    siheap_array_t *new_pair = source_pair_ptr(cur, NANBOX_OFNULL());
    if (prev_pair) {
      siarray_put(prev_pair, 1, SIHEAP_PTRTONANBOX(new_pair));
    }
    if (!new_list) {
      new_list = new_pair;
    }
    prev_pair = new_pair;
  }

  return SIHEAP_PTRTONANBOX(new_list);
}

static sinanbox_t sivmfn_prim_remove_all(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  if (NANBOX_ISNULL(argv[1])) {
    return NANBOX_OFNULL();
  }

  const sinanbox_t needle = argv[0];

  sinanbox_t list = argv[1];
  siheap_array_t *new_list = NULL;
  siheap_array_t *prev_pair = NULL;
  while (!NANBOX_ISNULL(list)) {
    siheap_array_t *pair = nanbox_toarray(list);
    sinanbox_t cur = siarray_get(pair, 0);
    list = siarray_get(pair, 1);
    if (sivm_equal(cur, needle)) {
      continue;
    }

    siheap_refbox(cur);
    siheap_array_t *new_pair = source_pair_ptr(cur, NANBOX_OFNULL());
    if (prev_pair) {
      siarray_put(prev_pair, 1, SIHEAP_PTRTONANBOX(new_pair));
    }
    if (!new_list) {
      new_list = new_pair;
      siheap_intref(new_list);
    }
    prev_pair = new_pair;
  }

  if (new_list) {
    siheap_intderef(new_list);
    return SIHEAP_PTRTONANBOX(new_list);
  }
  return NANBOX_OFNULL();
}

static sinanbox_t sivmfn_prim_reverse(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);

  sinanbox_t list = argv[0];
  if (NANBOX_ISNULL(list)) {
    return list;
  }

  siheap_array_t *new_list = NULL;

  while (!NANBOX_ISNULL(list)) {
    siheap_array_t *pair = nanbox_toarray(list);
    sinanbox_t cur = siarray_get(pair, 0);
    list = siarray_get(pair, 1);

    siheap_refbox(cur);
    new_list = source_pair_ptr(cur, new_list ? SIHEAP_PTRTONANBOX(new_list) : NANBOX_OFNULL());
  }

  return SIHEAP_PTRTONANBOX(new_list);
}

/******************************************************************************
 * Array primitive
 ******************************************************************************/

static sinanbox_t sivmfn_prim_array_length(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  sinanbox_t v = *argv;
  siheap_header_t *obj = SIHEAP_NANBOXTOPTR(v);
  if (!NANBOX_ISPTR(v) || obj->type != sitype_array) {
    sifault(sinter_fault_type);
    return NANBOX_OFEMPTY();
  }

  siheap_array_t *arr = (siheap_array_t *) obj;
  return NANBOX_WRAP_INT((int) arr->count);
}

/******************************************************************************
 * Stream primitives
 ******************************************************************************/

static inline sinanbox_t source_stream_tail(sinanbox_t stream) {
  sinanbox_t tail = source_tail(stream);
  return siexec_nanbox(tail, 0, NULL);
}

static sinanbox_t sivmfn_prim_list_to_stream(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);

  sinanbox_t list = argv[0];
  if (NANBOX_ISNULL(list)) {
    return list;
  }

  siheap_array_t *pair = nanbox_toarray(list);
  sinanbox_t head = siarray_get(pair, 0);
  sinanbox_t tail = siarray_get(pair, 1);
  siheap_refbox(head);
  siheap_refbox(tail);
  siheap_intcont_t *ic = siintcont_new(sivmfn_prim_list_to_stream, 2);
  ic->argv[0] = tail;
  return source_pair(head, SIHEAP_PTRTONANBOX(ic));
}

/**
 * Continuation for sivmfn_prim_build_stream.
 *
 * @param argc 2
 * @param argv <tt>{ current: number, max: number, fn: function }</tt>
 * @return <tt>pair(fn(current), intcont { current + 1, max, fn })</tt>
 */
static sinanbox_t prim_build_stream_cont(uint8_t argc, sinanbox_t *argv) {
  (void) argc;
  sinanbox_t fn = argv[2];
  int32_t cur = NANBOX_TOI32(argv[0]), max = NANBOX_TOI32(argv[1]);
  if (cur >= max) {
    return NANBOX_OFNULL();
  }

  sinanbox_t curv = siexec_nanbox(fn, 1, argv);
  siheap_intcont_t *ic = siintcont_new(prim_build_stream_cont, 3);
  ic->argv[0] = NANBOX_WRAP_INT(cur + 1);
  ic->argv[1] = argv[1];
  siheap_refbox(fn);
  ic->argv[2] = fn;

  return source_pair(curv, SIHEAP_PTRTONANBOX(ic));
}

static sinanbox_t sivmfn_prim_build_stream(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  sinanbox_t arg[] = { NANBOX_OFINT(0u), argv[0], argv[1] };
  return prim_build_stream_cont(3, arg);
}

static sinanbox_t sivmfn_prim_enum_stream(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  sinanbox_t start = argv[0], end = argv[1];

  if (NANBOX_ISINT(start) && NANBOX_ISINT(end)) {
    // n.b. don't merge this into an &&, we don't want to unnecessarily convert to float
    if (NANBOX_INT(start) > NANBOX_INT(end)) {
      return NANBOX_OFNULL();
    }
  } else if (NANBOX_TOFLOAT(start) > NANBOX_TOFLOAT(end)) {
    return NANBOX_OFNULL();
  }

  siheap_intcont_t *ic = siintcont_new(sivmfn_prim_enum_stream, 2);
  if (NANBOX_ISINT(start)) {
    ic->argv[0] = NANBOX_WRAP_INT(NANBOX_INT(start) + 1);
  } else { // note: type checked by the ISINT or TOFLOAT above
    ic->argv[0] = NANBOX_OFFLOAT(NANBOX_FLOAT(start) + 1);
  }
  ic->argv[1] = end;
  return source_pair(start, SIHEAP_PTRTONANBOX(ic));
}

static sinanbox_t sivmfn_prim_eval_stream(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);

  int32_t limit = NANBOX_TOI32(argv[1]);

  if (limit <= 0) {
    return NANBOX_OFNULL();
  }

  siheap_array_t *new_list = NULL;
  siheap_array_t *prev_pair = NULL;
  sinanbox_t stream = argv[0];
  siheap_refbox(stream);

  for (int32_t i = 0; i < limit; ++i) {
    siheap_array_t *stream_pair = nanbox_toarray(stream);
    sinanbox_t new_val = siarray_get(stream_pair, 0);
    sinanbox_t stream_tail = siarray_get(stream_pair, 1);
    siheap_intref(stream_pair);
    stream = siexec_nanbox(stream_tail, 0, NULL);
    siheap_intderef(stream_pair);

    siheap_refbox(new_val);
    siheap_deref(stream_pair);

    siheap_array_t *new_pair = source_pair_ptr(new_val, NANBOX_OFNULL());
    if (prev_pair) {
      siarray_put(prev_pair, 1, SIHEAP_PTRTONANBOX(new_pair));
    }
    if (!new_list) {
      new_list = new_pair;
      siheap_intref(new_list);
    }
    prev_pair = new_pair;
  }

  siheap_derefbox(stream);
  siheap_intderef(new_list);
  return SIHEAP_PTRTONANBOX(new_list);
}

static sinanbox_t sivmfn_prim_integers_from(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);

  sinanbox_t val = argv[0];
  siheap_intcont_t *ic = siintcont_new(sivmfn_prim_integers_from, 1);
  if (NANBOX_ISINT(val)) {
    ic->argv[0] = NANBOX_WRAP_INT(NANBOX_INT(val) + 1);
  } else {
    ic->argv[0] = NANBOX_OFFLOAT(NANBOX_TOFLOAT(val) + 1);
  }

  return source_pair(val, SIHEAP_PTRTONANBOX(ic));
}

/**
 * Continuation for sivmfn_prim_stream.
 *
 * @param argc 1 | 2
 * @param argv <tt>{ array: array, next_index?: number }</tt>
 * @return <tt>pair(array[next_index], intcont { array, next_index + 1 })</tt>
 */
static sinanbox_t prim_stream_cont(uint8_t argc, sinanbox_t *argv) {
  (void) argc;
  // no array (this is an optimisation for a stream of 1)
  if (NANBOX_ISNULL(argv[0])) {
    return NANBOX_OFNULL();
  }

  uint32_t idx = NANBOX_TOU32(argv[1]);
  siheap_array_t *arr = SIHEAP_NANBOXTOPTR(argv[0]);

  if (idx >= arr->count) {
    return NANBOX_OFNULL();
  }

  // construct the new continuation
  siheap_intcont_t *ic = siintcont_new(prim_stream_cont, 2);
  ic->argv[0] = argv[0];
  ic->argv[1] = NANBOX_WRAP_UINT(idx + 1);

  // ref the new pair's head
  siheap_refbox(arr->data->data[idx]);
  // ref the array (since it's going into the continuation)
  siheap_ref(arr);

  return source_pair(arr->data->data[idx], SIHEAP_PTRTONANBOX(ic));
}

static sinanbox_t sivmfn_prim_stream(uint8_t argc, sinanbox_t *argv) {
  if (!argc) {
    return NANBOX_OFNULL();
  } else if (argc == 1) {
    siheap_intcont_t *ic = siintcont_new(prim_stream_cont, 1);
    ic->argv[0] = NANBOX_OFNULL();
    siheap_refbox(argv[0]);
    return source_pair(argv[0], SIHEAP_PTRTONANBOX(ic));
  }

  for (size_t i = 0; i < argc; ++i) {
    siheap_refbox(argv[i]);
  }

  siheap_array_t *arr = siarray_new(argc - 1);
  memcpy(arr->data->data, argv + 1, (argc - 1)*sizeof(sinanbox_t));
  arr->count = argc - 1;

  siheap_intcont_t *ic = siintcont_new(prim_stream_cont, 2);
  ic->argv[0] = SIHEAP_PTRTONANBOX(arr);
  ic->argv[1] = NANBOX_OFINT(0);
  return source_pair(argv[0], SIHEAP_PTRTONANBOX(ic));
}

static sinanbox_t sivmfn_prim_stream_append(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_stream_filter(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_stream_for_each(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);

  sinanbox_t stream = argv[1];
  if (NANBOX_ISNULL(stream)) {
    return NANBOX_OFUNDEF();
  }

  sinanbox_t fn = argv[0];

  siheap_refbox(stream);
  while (!NANBOX_ISNULL(stream)) {
    siheap_array_t *stream_pair = nanbox_toarray(stream);
    sinanbox_t head = siarray_get(stream_pair, 0);
    sinanbox_t stream_tail = siarray_get(stream_pair, 1);
    siheap_intref(stream_pair);
    siheap_refbox(head);
    siheap_derefbox(siexec_nanbox(fn, 1, &head));
    stream = siexec_nanbox(stream_tail, 0, NULL);
    siheap_intderef(stream_pair);
    siheap_deref(stream_pair);
  }
  siheap_derefbox(stream);

  return NANBOX_OFUNDEF();
}

static sinanbox_t sivmfn_prim_stream_length(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);

  sinanbox_t stream = argv[0];
  if (NANBOX_ISNULL(stream)) {
    return NANBOX_OFINT(0);
  }

  size_t length = 0;
  siheap_refbox(stream);
  while (!NANBOX_ISNULL(stream)) {
    ++length;
    sinanbox_t last_stream = stream;
    siheap_intrefbox(last_stream);
    stream = source_stream_tail(last_stream);
    siheap_intderefbox(last_stream);
    siheap_derefbox(last_stream);
  }
  siheap_derefbox(stream);

  return NANBOX_WRAP_UINT(length);
}

static sinanbox_t sivmfn_prim_stream_map(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_stream_member(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_stream_ref(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_stream_remove(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_stream_remove_all(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_stream_reverse(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_stream_tail(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  return source_stream_tail(argv[0]);
}

static sinanbox_t sivmfn_prim_stream_to_list(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);

  sinanbox_t stream = argv[0];

  if (NANBOX_ISNULL(stream)) {
    return NANBOX_OFNULL();
  }

  siheap_array_t *new_list = NULL;
  siheap_array_t *prev_pair = NULL;
  siheap_refbox(stream);
  while (!NANBOX_ISNULL(stream)) {
    siheap_array_t *stream_pair = nanbox_toarray(stream);
    sinanbox_t new_val = siarray_get(stream_pair, 0);
    sinanbox_t stream_tail = siarray_get(stream_pair, 1);
    siheap_intref(stream_pair);
    stream = siexec_nanbox(stream_tail, 0, NULL);
    siheap_intderef(stream_pair);

    siheap_refbox(new_val);
    siheap_deref(stream_pair);

    siheap_array_t *new_pair = source_pair_ptr(new_val, NANBOX_OFNULL());
    if (prev_pair) {
      siarray_put(prev_pair, 1, SIHEAP_PTRTONANBOX(new_pair));
    }
    if (!new_list) {
      new_list = new_pair;
      siheap_intref(new_list);
    }
    prev_pair = new_pair;
  }

  siheap_derefbox(stream);
  siheap_intderef(new_list);
  return SIHEAP_PTRTONANBOX(new_list);
}

/******************************************************************************
 * Miscellaneous primitives
 ******************************************************************************/

static sinanbox_t sivmfn_prim_display(uint8_t argc, sinanbox_t *argv) {
  SIDEBUG("Program called display with %d arguments:\n", argc);
  debug_display_argv(argc, argv);
  handle_display(argc, argv, false);
  if (argc) {
    siheap_refbox(argv[0]);
    return argv[0];
  }
  return NANBOX_OFUNDEF();
}

static inline bool structural_equal(sinanbox_t l, sinanbox_t r) {
  if (sivm_equal(l, r)) {
    return true;
  }

  if (!NANBOX_ISPTR(l) || !NANBOX_ISPTR(r)) {
    return false;
  }

  siheap_header_t *lv = SIHEAP_NANBOXTOPTR(l);
  siheap_header_t *rv = SIHEAP_NANBOXTOPTR(r);
  if (lv->type != sitype_array || rv->type != sitype_array) {
    return false;
  }

  siheap_array_t *la = (siheap_array_t *) lv;
  siheap_array_t *ra = (siheap_array_t *) rv;
  if (la->count != 2 || ra->count != 2) {
    return false;
  }

  return structural_equal(siarray_get(la, 0), siarray_get(ra, 0))
    && structural_equal(siarray_get(la, 1), siarray_get(ra, 1));
}

static sinanbox_t sivmfn_prim_equal(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(2);
  return NANBOX_OFBOOL(structural_equal(argv[0], argv[1]));
}

static sinanbox_t sivmfn_prim_error(uint8_t argc, sinanbox_t *argv) {
  SIDEBUG("Program called error with %d arguments:\n", argc);
  debug_display_argv(argc, argv);
  handle_display(argc, argv, true);
  sifault(sinter_fault_program_error);
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_unimpl(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  SIBUGV("Unimplemented primitive function %02x at address 0x%tx\n", *(sistate.pc + 1), SISTATE_CURADDR);
  sifault(sinter_fault_invalid_program);
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_noop(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  return NANBOX_OFUNDEF();
}

const sivmfnptr_t sivmfn_primitives[] = {
  sivmfn_prim_accumulate,
  sivmfn_prim_append,
  sivmfn_prim_array_length,
  sivmfn_prim_build_list,
  sivmfn_prim_build_stream,
  sivmfn_prim_display,
  /* draw_data */ sivmfn_prim_noop, // not supported, obviously
  sivmfn_prim_enum_list,
  sivmfn_prim_enum_stream,
  sivmfn_prim_equal,
  sivmfn_prim_error,
  sivmfn_prim_eval_stream,
  sivmfn_prim_filter,
  sivmfn_prim_for_each,
  sivmfn_prim_head,
  sivmfn_prim_integers_from,
  sivmfn_prim_is_array,
  sivmfn_prim_is_boolean,
  sivmfn_prim_is_function,
  sivmfn_prim_is_list,
  sivmfn_prim_is_null,
  sivmfn_prim_is_number,
  sivmfn_prim_is_pair,
  sivmfn_prim_is_stream,
  sivmfn_prim_is_string,
  sivmfn_prim_is_undefined,
  sivmfn_prim_length,
  sivmfn_prim_list,
  sivmfn_prim_list_ref,
  sivmfn_prim_list_to_stream,
  sivmfn_prim_list_to_string,
  sivmfn_prim_map,
  sivmfn_prim_math_abs,
  sivmfn_prim_math_acos,
  sivmfn_prim_math_acosh,
  sivmfn_prim_math_asin,
  sivmfn_prim_math_asinh,
  sivmfn_prim_math_atan,
  sivmfn_prim_math_atan2,
  sivmfn_prim_math_atanh,
  sivmfn_prim_math_cbrt,
  sivmfn_prim_math_ceil,
  sivmfn_prim_math_clz32,
  sivmfn_prim_math_cos,
  sivmfn_prim_math_cosh,
  sivmfn_prim_math_exp,
  sivmfn_prim_math_expm1,
  sivmfn_prim_math_floor,
  sivmfn_prim_math_fround,
  sivmfn_prim_math_hypot,
  sivmfn_prim_math_imul,
  sivmfn_prim_math_log,
  sivmfn_prim_math_log1p,
  sivmfn_prim_math_log2,
  sivmfn_prim_math_log10,
  sivmfn_prim_math_max,
  sivmfn_prim_math_min,
  sivmfn_prim_math_pow,
  sivmfn_prim_math_random,
  sivmfn_prim_math_round,
  sivmfn_prim_math_sign,
  sivmfn_prim_math_sin,
  sivmfn_prim_math_sinh,
  sivmfn_prim_math_sqrt,
  sivmfn_prim_math_tan,
  sivmfn_prim_math_tanh,
  sivmfn_prim_math_trunc,
  sivmfn_prim_member,
  sivmfn_prim_pair,
  /* parse_int */ sivmfn_prim_unimpl, // TODO: doesn't make sense without the ability to take input (prompt)
  sivmfn_prim_remove,
  sivmfn_prim_remove_all,
  sivmfn_prim_reverse,
  /* runtime */ sivmfn_prim_unimpl, // TODO: need to get time from host
  sivmfn_prim_set_head,
  sivmfn_prim_set_tail,
  sivmfn_prim_stream,
  sivmfn_prim_stream_append,
  sivmfn_prim_stream_filter,
  sivmfn_prim_stream_for_each,
  sivmfn_prim_stream_length,
  sivmfn_prim_stream_map,
  sivmfn_prim_stream_member,
  sivmfn_prim_stream_ref,
  sivmfn_prim_stream_remove,
  sivmfn_prim_stream_remove_all,
  sivmfn_prim_stream_reverse,
  sivmfn_prim_stream_tail,
  sivmfn_prim_stream_to_list,
  sivmfn_prim_tail,
  /* stringify */ sivmfn_prim_unimpl, // TODO: do we want this?
  /* prompt */ sivmfn_prim_unimpl // TODO: need to call out to host
};
