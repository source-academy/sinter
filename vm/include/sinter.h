#ifndef SINTER_H
#define SINTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
  sinter_fault_internal_error = 9,
  sinter_fault_function_arity = 10,
  sinter_fault_program_error = 11,
  sinter_fault_uninitialised_heap = 12
} sinter_fault_t;

typedef struct {
  sinter_type_t type;
  union {
    bool boolean_value;
    int32_t integer_value;
    float float_value;
    const char *string_value;
    uint32_t object_value;
  };
} sinter_value_t;

/**
 * Runs a program.
 *
 * This is the main entrypoint for programs using the Sinter VM as a library.
 */
sinter_fault_t sinter_run(const unsigned char *code, const size_t code_size, sinter_value_t *result);

/**
 * Set up the heap.
 *
 * This function is a no-op if SINTER_STATIC_HEAP is defined.
 */
void sinter_setup_heap(void *heap, size_t size);

/**
 * The type of a string printer function.
 *
 * A newline should not be appended by the function.
 */
typedef void (*sinter_printfn_string)(const char *str, bool is_error);

/**
 * The type of an integer printer function.
 *
 * A newline should not be appended by the function.
 */
typedef void (*sinter_printfn_integer)(int32_t value, bool is_error);

/**
 * The type of a float printer function.
 *
 * A newline should not be appended by the function.
 */
typedef void (*sinter_printfn_float)(float value, bool is_error);

extern sinter_printfn_string sinter_printer_string;
extern sinter_printfn_integer sinter_printer_integer;
extern sinter_printfn_float sinter_printer_float;

#ifdef __cplusplus
}
#endif

#endif
