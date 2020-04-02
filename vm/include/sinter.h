#ifndef SINTER_H
#define SINTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  sinter_type_undefined = 1,
  sinter_type_null = 2,
  sinter_type_boolean = 3,
  sinter_type_integer = 4,
  sinter_type_float = 5,
  sinter_type_string = 6,
  sinter_type_array = 7,
  sinter_type_function = 8
} sinter_type_t;

typedef enum {
  sinter_fault_none = 0,
  sinter_fault_out_of_memory = 1,
  sinter_fault_type = 2,
  sinter_fault_divide_by_zero = 3,
  sinter_fault_stack_overflow = 4,
  sinter_fault_stack_underflow = 5,
  sinter_fault_uninitialised_load = 6,
  sinter_fault_invalid_load = 7,
  sinter_fault_invalid_program = 8,
  sinter_fault_internal_error = 9
} sinter_fault_t;

typedef struct {
  sinter_type_t type;
  union {
    bool boolean_value;
    int32_t integer_value;
    float float_value;
  };
} sinter_value_t;

sinter_fault_t sinter_run(const unsigned char *code, const size_t code_size, sinter_value_t *result);

#endif
