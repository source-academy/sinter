#ifndef SINTER_PROGRAM_H
#define SINTER_PROGRAM_H

#include "config.h"

#include <stdint.h>

#include "opcode.h"

#define SVM_MAGIC 0x5005ACADu

/**
 * The header of an SVM program.
 */
typedef struct __attribute__((__packed__)) {
  /**
   * The magic. Should be 0x5005ACAD.
   */
  uint32_t magic;
  /**
   * The major version.
   */
  uint16_t v_major;
  /**
   * The minor version.
   */
  uint16_t v_minor;
  /**
   * The address to the entry point [svm_function](@ref svm_function).
   */
  address_t entry;
  /**
   * The number of constants following the header.
   */
  uint32_t constant_count;
} svm_header_t;
_Static_assert(sizeof(svm_header_t) == 16, "Wrong svm_header_t size");

/**
 * A constant in the constant pool of an SVM program.
 */
typedef struct __attribute__((__packed__)) {
  uint16_t type;
  uint32_t length;
#ifndef __cplusplus
  unsigned char data[];
#endif
} svm_constant_t;
_Static_assert(sizeof(svm_constant_t) == 6, "Wrong svm_constant_t size");

/**
 * A function in an SVM program. All code in an SVM program is in a function.
 */
typedef struct __attribute__((__packed__)) {
  /**
   * The maximum stack size used by the function.
   */
  uint8_t stack_size;
  /**
   * The number of environment entries used by the function.
   */
  uint8_t env_size;
  /**
   * The number of arguments expected by the function.
   */
  uint8_t num_args;
  uint8_t padding;
  /**
   * The first instruction.
   */
  opcode_t code;
} svm_function_t;
_Static_assert(sizeof(svm_function_t) == 5, "Wrong svm_function_t size");

#endif
