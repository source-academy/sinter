#ifndef SINTER_HEAP_OBJ_H
#define SINTER_HEAP_OBJ_H

#include "config.h"
#include "opcode.h"
#include "heap.h"
#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
struct siheap_env;
typedef siheap_env siheap_env_t;
#else
typedef struct siheap_env {
  siheap_header_t header;
  struct siheap_env *parent;
  uint16_t entry_count;
  sinanbox_t entry[];
} siheap_env_t;
#endif

#define SIENV_SIZE(entry_count) (sizeof(siheap_env_t) + (entry_count)*sizeof(sinanbox_t))

/**
 * Create a new environment heap object.
 *
 * This increments the reference count on the parent environment, if any.
 */
SINTER_INLINEIFC siheap_env_t *sienv_new(
  siheap_env_t *parent,
  const uint16_t entry_count) SINTER_BODYIFC(
  siheap_env_t *env = (siheap_env_t *) siheap_malloc(SIENV_SIZE(entry_count), sitype_env);
  env->parent = parent;
  env->entry_count = entry_count;
  for (size_t i = 0; i < entry_count; ++i) {
    env->entry[i] = NANBOX_OFEMPTY();
  }
  if (parent) {
    siheap_ref(parent);
  }

  return env;
)

SINTER_INLINEIFC void sienv_destroy(siheap_env_t *const env) SINTER_BODYIFC(
  for (size_t i = 0; i < env->entry_count; ++i) {
    siheap_derefbox(env->entry[i]);
  }
  if (env->parent) {
    siheap_deref(env->parent);
  }
)

/**
 * Get a value from the environment.
 *
 * Note: the caller is responsible for incrementing the reference count, if needed.
 */
SINTER_INLINEIFC sinanbox_t sienv_get(
  siheap_env_t *const env,
  const uint16_t index) SINTER_BODYIFC(

SINTER_BODYIFSB(
  if (index >= env->entry_count) {
    sifault(sinter_fault_invalid_load);
    return NANBOX_OFEMPTY();
  }
)

  return env->entry[index];
)

/**
 * Put a value into the environment.
 *
 * If a heap pointer is replaced, the heap object's reference count is decremented.
 *
 * Note: the caller "passes" its reference to the environment. That is, the caller should increment the reference
 * count of the heap object (if the value is a pointer) if it is going to continue holding on to the value.
 */
SINTER_INLINEIFC void sienv_put(
  siheap_env_t *const env,
  const uint16_t index,
  const sinanbox_t val) SINTER_BODYIFC(

SINTER_BODYIFSB(
  if (index >= env->entry_count) {
    sifault(sinter_fault_invalid_load);
    return;
  }
)

  siheap_derefbox(env->entry[index]);
  env->entry[index] = val;
)

SINTER_INLINEIFC siheap_env_t *sienv_getparent(
  siheap_env_t *env,
  unsigned int index) SINTER_BODYIFC(
  while (env && index--) {
    env = env->parent;
  }
  return env;
)

typedef struct {
  siheap_header_t header;
  const svm_function_t *code;
  siheap_env_t *env;
} siheap_function_t;

SINTER_INLINE siheap_function_t *sifunction_new(const svm_function_t *code, siheap_env_t *parent_env) {
  siheap_function_t *fn = (siheap_function_t *) siheap_malloc(sizeof(siheap_function_t), sinter_type_function);
  fn->code = code;
  fn->env = parent_env;
  siheap_ref(parent_env);

  return fn;
}

SINTER_INLINE void sifunction_destroy(siheap_function_t *fn) {
  if (fn->env) {
    siheap_deref(fn->env);
  }
}

typedef struct {
  siheap_header_t header;
  const opcode_t *return_address;
  sinanbox_t *saved_stack_bottom;
  sinanbox_t *saved_stack_limit;
  sinanbox_t *saved_stack_top;
  siheap_env_t *saved_env;
} siheap_frame_t;

SINTER_INLINE siheap_frame_t *siframe_new(void) {
  return (siheap_frame_t *) siheap_malloc(
    sizeof(siheap_frame_t), sitype_frame);
}

typedef struct {
  siheap_header_t header;
  const svm_constant_t *string;
} siheap_strconst_t;

SINTER_INLINE siheap_strconst_t *sistrconst_new(const svm_constant_t *string) {
  siheap_strconst_t *obj = (siheap_strconst_t *) siheap_malloc(sizeof(siheap_strconst_t), sitype_strconst);
  obj->string = string;

  return obj;
}

typedef struct {
  siheap_header_t header;
  siheap_header_t *left;
  siheap_header_t *right;
} siheap_strpair_t;

/**
 * Creates a new string pair, representing concatenation.
 *
 * The refcount of left and right are incremented.
 */
SINTER_INLINE siheap_strpair_t *sistrpair_new(siheap_header_t *left, siheap_header_t *right) {
  siheap_strpair_t *obj = (siheap_strpair_t *) siheap_malloc(sizeof(siheap_strpair_t), sitype_strpair);
  obj->left = left;
  obj->right = right;

  siheap_ref(left);
  siheap_ref(right);

  return obj;
}

SINTER_INLINE void sistrpair_destroy(siheap_strpair_t *obj) {
  siheap_deref(obj->left);
  siheap_deref(obj->right);
}

#ifdef __cplusplus
struct siheap_string;
typedef struct siheap_string siheap_string_t;
#else
typedef struct siheap_string {
  siheap_header_t header;
  char string[];
} siheap_string_t;
#endif

SINTER_INLINEIFC siheap_string_t *sistring_new(address_t size) SINTER_BODYIFC(
  siheap_string_t *obj = (siheap_string_t *) siheap_malloc(sizeof(siheap_string_t) + size, sitype_string);
  return obj;
)

siheap_string_t *sistrpair_flatten(siheap_strpair_t *obj);

SINTER_INLINE const char *sistrobj_tocharptr(siheap_header_t *obj) {
  switch (obj->type) {
  case sitype_strconst: {
    siheap_strconst_t *v = (siheap_strconst_t *) obj;
    return (const char *) v->string->data;
  }

  case sitype_strpair: {
    siheap_strpair_t *v = (siheap_strpair_t *) obj;
    siheap_string_t *str = sistrpair_flatten(v);
    return str->string;
  }

  case sitype_string: {
    siheap_string_t *v = (siheap_string_t *) obj;
    return v->string;
  }

  default:
    SIBUGM("Unknown string type\n");
    sifault(sinter_fault_internal_error);
    break;
  }
}

#ifdef __cplusplus
}
#endif

#endif
