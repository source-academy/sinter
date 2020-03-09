#ifndef SINTER_PROGRAM_H
#define SINTER_PROGRAM_H

#include <stdint.h>

#include "opcode.h"

#define SVM_MAGIC 0x5005ACADu

/**
 * The header of an SVM program.
 */
struct __attribute__((__packed__)) svm_header {
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
};
_Static_assert(sizeof(struct svm_header) == 16, "Wrong struct svm_header size");

/**
 * A constant in the constant pool of an SVM program.
 */
struct __attribute__((__packed__)) svm_constant {
  uint16_t type;
  uint32_t length;
  unsigned char data;
};
_Static_assert(sizeof(struct svm_constant) == 7, "Wrong struct svm_constant size");

/**
 * A function in an SVM program. All code in an SVM program is in a function.
 */
struct __attribute__((__packed__)) svm_function {
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
};
_Static_assert(sizeof(struct svm_function) == 5, "Wrong struct svm_function size");

#endif
