#ifndef SINTER_HEAP_OBJ_H
#define SINTER_HEAP_OBJ_H

#include "opcode.h"
#include "heap.h"

struct siheap_env {
  struct siheap_header header;
  struct siheap_env *parent;
  uint16_t entry_count;
  sinanbox_t entry[];
};

#define SIENV_SIZE(entry_count) (sizeof(struct siheap_env) + (entry_count)*sizeof(sinanbox_t))

static inline struct siheap_env *sienv_new(
  struct siheap_env *parent,
  const uint16_t entry_count) {
  struct siheap_env *env = (struct siheap_env *) siheap_malloc(SIENV_SIZE(entry_count), sitype_env);
  env->parent = parent;
  env->entry_count = entry_count;
  for (size_t i = 0; i < entry_count; ++i) {
    env->entry[i] = NANBOX_OFEMPTY();
  }

  return env;
}

/**
 * Get a value from the environment.
 *
 * Note: he caller is responsible for incrementing the reference count, if needed.
 */
static inline sinanbox_t sienv_get(
  struct siheap_env *const env,
  const uint16_t index) {

#ifndef SINTER_SEATBELTS_OFF
  if (index >= env->entry_count) {
    sifault(sinter_fault_invalid_load);
    return NANBOX_OFEMPTY();
  }
#endif

  return env->entry[index];
}

/**
 * Put a value into the environment.
 *
 * If a heap pointer is replaced, the heap object's reference count is decremented.
 *
 * Note: the caller "passes" its reference to the environment. That is, the caller should increment the reference
 * count of the heap object (if the value is a pointer) if it is going to continue holding on to the value.
 */
static inline void sienv_put(
  struct siheap_env *const env,
  const uint16_t index,
  const sinanbox_t val) {

#ifndef SINTER_SEATBELTS_OFF
  if (index >= env->entry_count) {
    sifault(sinter_fault_invalid_load);
    return;
  }
#endif

  siheap_derefbox(env->entry[index]);
  env->entry[index] = val;
}

static inline struct siheap_env *sienv_getparent(
  struct siheap_env *env,
  unsigned int index) {
  while (env && index--) {
    env = env->parent;
  }
  return env;
}

struct siheap_function {
  struct siheap_header header;
  const struct svm_function *code;
  struct siheap_env *env;
};

static inline struct siheap_function *sifunction_new(const struct svm_function *code, struct siheap_env *parent_env) {
  struct siheap_function *fn = (struct siheap_function *) siheap_malloc(sizeof(struct siheap_function), sinter_type_function);
  fn->code = code;
  fn->env = parent_env;
  siheap_ref(parent_env);

  return fn;
}

struct siheap_frame {
  struct siheap_header header;
  const opcode_t *return_address;
  sinanbox_t *saved_stack_bottom;
  sinanbox_t *saved_stack_limit;
  sinanbox_t *saved_stack_top;
  struct siheap_env *saved_env;
};

static inline struct siheap_frame *siframe_new(void) {
  return (struct siheap_frame *) siheap_malloc(
    sizeof(struct siheap_frame), sitype_frame);
}

#endif
