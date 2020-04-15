#include <stdint.h>
#include <math.h>

#include <sinter/nanbox.h>
#include <sinter/fault.h>
#include <sinter/heap.h>
#include <sinter/stack.h>
#include <sinter/debug.h>
#include <sinter/vm.h>

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

static void display_strobj(siheap_header_t *obj, bool is_error) {
  switch (obj->type) {
  case sitype_strconst:
    SIVMFN_PRINT((const char *) ((siheap_strconst_t *) obj)->string->data, is_error);
    break;
  case sitype_strpair: {
    siheap_strpair_t *pair = (siheap_strpair_t *) obj;
    display_strobj(pair->left, is_error);
    display_strobj(pair->right, is_error);
    break;
  }
  }
}

static void display_nanbox(sinanbox_t v, bool is_error) {
  switch (NANBOX_GETTYPE(v)) {
  NANBOX_CASES_TINT
    SIVMFN_PRINT((int32_t) NANBOX_INT(v), is_error);
    break;
  case NANBOX_TBOOL:
    SIVMFN_PRINT(NANBOX_BOOL(v) ? "true" : "false", is_error);
    break;
  case NANBOX_TUNDEF:
    SIVMFN_PRINT("undefined", is_error);
    break;
  case NANBOX_TNULL:
    SIVMFN_PRINT("null", is_error);
    break;
  NANBOX_CASES_TPTR {
    siheap_header_t *obj = SIHEAP_NANBOXTOPTR(v);
    switch (obj->type) {
    case sitype_strconst:
    case sitype_strpair:
      display_strobj(obj, is_error);
      break;
    default:
      SIBUGM("Unexpected object type\n");
      break;
    }
    break;
  }
  default:
    if (NANBOX_ISFLOAT(v)) {
      SIVMFN_PRINT(NANBOX_FLOAT(v), is_error);
    } else {
      SIBUGM("Unexpected type\n");
    }
    break;
  }
}

static void handle_display(unsigned int argc, sinanbox_t *argv, bool is_error) {
  if (argc < 1) {
    return;
  }

  if (argc > 1) {
    display_nanbox(argv[1], is_error);
    SIVMFN_PRINT(" ", is_error);
  }

  display_nanbox(argv[0], is_error);
  SIVMFN_PRINT("\n", is_error);
}

#define CHECK_ARGC(n) do { \
  if (argc != (n)) { \
    sifault(sinter_fault_function_arity); \
    return NANBOX_OFEMPTY(); \
  } \
} while (0)

static sinanbox_t sivmfn_prim_accumulate(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_append(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_array_length(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_build_list(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_build_stream(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_display(uint8_t argc, sinanbox_t *argv) {
  SIDEBUG("Program called display with %d arguments:\n", argc);
  debug_display_argv(argc, argv);
  handle_display(argc, argv, false);
  return argc ? argv[0] : NANBOX_OFUNDEF();
}

static sinanbox_t sivmfn_prim_draw_data(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_enum_list(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_enum_stream(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_equal(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_error(uint8_t argc, sinanbox_t *argv) {
  SIDEBUG("Program called error with %d arguments:\n", argc);
  debug_display_argv(argc, argv);
  handle_display(argc, argv, true);
  sifault(sinter_fault_program_error);
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_eval_stream(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_filter(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_for_each(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_head(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_integers_from(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_is_array(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_is_boolean(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  return NANBOX_OFBOOL(NANBOX_ISBOOL(*argv));
}

static sinanbox_t sivmfn_prim_is_function(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  sinanbox_t v = *argv;
  return NANBOX_OFBOOL((NANBOX_ISPTR(v) && ((siheap_header_t *) SIHEAP_NANBOXTOPTR(v))->type == sinter_type_function)
    || NANBOX_ISIFN(v));
}

static sinanbox_t sivmfn_prim_is_list(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_is_null(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  return NANBOX_OFBOOL(NANBOX_ISNULL(*argv));
}

static sinanbox_t sivmfn_prim_is_number(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  return NANBOX_OFBOOL(NANBOX_ISNUMERIC(*argv));
}

static sinanbox_t sivmfn_prim_is_pair(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_is_stream(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_is_string(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_is_undefined(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  return NANBOX_OFBOOL(NANBOX_ISUNDEF(*argv));
}

static sinanbox_t sivmfn_prim_length(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_list(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_list_ref(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_list_to_stream(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_list_to_string(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_map(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_abs(uint8_t argc, sinanbox_t *argv) {
  CHECK_ARGC(1);
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_acos(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_acosh(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_asin(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_asinh(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_atan(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_atan2(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_atanh(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_cbrt(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_ceil(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_clz32(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_cos(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_cosh(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_exp(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_expm1(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_floor(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_fround(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_hypot(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_imul(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_log(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_log1p(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_log2(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_log10(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_max(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_min(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_pow(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_random(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_round(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_sign(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_sin(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_sinh(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_sqrt(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_tan(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_tanh(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_math_trunc(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_member(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_pair(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_parse_int(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_remove(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_remove_all(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_reverse(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_runtime(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_set_head(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_set_tail(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_stream(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
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
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_stream_length(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
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
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_stream_to_list(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_tail(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_stringify(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

static sinanbox_t sivmfn_prim_prompt(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  unimpl();
  return NANBOX_OFEMPTY();
}

const sivmfnptr_t sivmfn_primitives[] = {
  sivmfn_prim_accumulate,
  sivmfn_prim_append,
  sivmfn_prim_array_length,
  sivmfn_prim_build_list,
  sivmfn_prim_build_stream,
  sivmfn_prim_display,
  sivmfn_prim_draw_data,
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
  sivmfn_prim_parse_int,
  sivmfn_prim_remove,
  sivmfn_prim_remove_all,
  sivmfn_prim_reverse,
  sivmfn_prim_runtime,
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
  sivmfn_prim_stringify,
  sivmfn_prim_prompt
};
