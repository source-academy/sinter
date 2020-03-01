#ifndef SINTER_H
#define SINTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum sinter_type {
  sinter_type_undefined = 1,
  sinter_type_null = 2,
  sinter_type_boolean = 3,
  sinter_type_integer = 4,
  sinter_type_float = 5,
  sinter_type_string = 6,
  sinter_type_array = 7,
  sinter_type_function = 8
};

enum sinter_fault {
  // skip 0 (setjmp returns 0 on normal return)
  sinter_fault_none = 1,
  sinter_fault_oom = 2,
  sinter_fault_type = 3,
  sinter_fault_divbyzero = 4,
  sinter_fault_stackoverflow = 5,
  sinter_fault_stackunderflow = 6,
  sinter_fault_uninitld = 7,
  sinter_fault_invalidld = 8
};

struct sinter_value {
  enum sinter_type type;
  union {
    bool boolean_value;
    int32_t integer_value;
    float float_value;
  };
};

static inline enum sinter_fault sinter_run(const unsigned char *code, struct sinter_value *result) {
  // stub
  (void) code;

  result->type = sinter_type_integer;
  result->integer_value = 42;
  return sinter_fault_none;
}

#endif
