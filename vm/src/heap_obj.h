#ifndef SINTER_HEAP_OBJ_H
#define SINTER_HEAP_OBJ_H

#include "opcode.h"
#include "heap.h"

struct siheap_env {
  struct siheap_header header;
  uint16_t localcount;
  uint16_t argcount;
  sinanbox_t entry[];
};

#define SIENV_SIZE(entrycount) (sizeof(struct siheap_env) + (entrycount)*sizeof(sinanbox_t))

static inline struct siheap_env *sienv_alloc(
  const uint16_t localcount,
  const uint16_t argcount) {
  struct siheap_env *env = (struct siheap_env *) siheap_malloc(SIENV_SIZE(localcount + argcount), sitype_env);
  if (!env) {
    return env;
  }

  env->localcount = localcount;
  env->argcount = argcount;
  for (size_t i = 0; i < localcount + argcount; ++i) {
    env->entry[i] = NANBOX_OFEMPTY();
  }

  return env;
}

static inline sinanbox_t *sienv_getlocal(
  struct siheap_env *const env,
  const uint16_t index) {

#ifndef SINTER_SEATBELTS_OFF
  if (index >= env->localcount) {
    sifault(sinter_fault_invalidld);
    return NULL;
  }
#endif

  return &env->entry[index];
}

static inline sinanbox_t *sienv_getarg(
  struct siheap_env *const env,
  const uint16_t index) {

#ifndef SINTER_SEATBELTS_OFF
  if (index >= env->argcount) {
    sifault(sinter_fault_invalidld);
    return NULL;
  }
#endif

  return &env->entry[env->localcount + index];
}

static inline void sienv_putlocal(
  struct siheap_env *const env,
  const uint16_t index,
  const sinanbox_t val) {

#ifndef SINTER_SEATBELTS_OFF
  if (index >= env->localcount) {
    sifault(sinter_fault_invalidld);
    return;
  }
#endif

  env->entry[index] = val;
}

static inline void sienv_putarg(
  struct siheap_env *const env,
  const uint16_t index,
  const sinanbox_t val) {

#ifndef SINTER_SEATBELTS_OFF
  if (index >= env->argcount) {
    sifault(sinter_fault_invalidld);
    return;
  }
#endif

  env->entry[env->localcount + index] = val;
}

struct siheap_function {
  struct siheap_header header;
  opcode_t *code;
  struct siheap_environment *env;
};

struct siheap_frame {
  struct siheap_header header;
  opcode_t *return_address;
  sinanbox_t *saved_stack_bottom;
  sinanbox_t *saved_stack_limit;
  sinanbox_t *saved_stack_top;
  struct siheap_env *saved_env;
};

static inline struct siheap_frame *siframe_alloc(void) {
  return (struct siheap_frame *) siheap_malloc(
    sizeof(struct siheap_frame), sitype_frame);
}

#endif
