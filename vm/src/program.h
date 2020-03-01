#ifndef SINTER_PROGRAM_H
#define SINTER_PROGRAM_H

#include <stdint.h>

#include "opcode.h"

#define SVM_MAGIC 0x5005ACADu

struct __attribute__((__packed__)) svm_header {
  uint32_t magic;
  uint16_t v_major;
  uint16_t v_minor;
  address_t entry;
  uint32_t constant_count;
};
_Static_assert(sizeof(struct svm_header) == 16, "Wrong struct svm_header size");

struct __attribute__((__packed__)) svm_constant {
  uint16_t type;
  uint32_t length;
  unsigned char data;
};
_Static_assert(sizeof(struct svm_constant) == 7, "Wrong struct svm_constant size");

struct __attribute__((__packed__)) svm_function {
  uint8_t stack_size;
  uint8_t env_size;
  uint8_t num_args;
  uint8_t padding;
  opcode_t code;
};
_Static_assert(sizeof(struct svm_function) == 5, "Wrong struct svm_function size");

#endif
