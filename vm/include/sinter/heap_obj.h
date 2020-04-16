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

SINTER_INLINEIFC const char *sistrobj_tocharptr(siheap_header_t *obj) SINTER_BODYIFC(
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
)

#ifdef __cplusplus
struct siheap_array_data;
typedef struct siheap_array_data siheap_array_data_t;
#else
typedef struct siheap_array_data {
  siheap_header_t header;
  sinanbox_t data[];
} siheap_array_data_t;
#endif

typedef struct {
  siheap_header_t header;
  address_t alloc_size;
  address_t count;
  siheap_array_data_t *data;
} siheap_array_t;

SINTER_INLINEIFC siheap_array_t *siarray_new(address_t alloc_size) SINTER_BODYIFC(
  siheap_array_t *array = (siheap_array_t *) siheap_malloc(sizeof(siheap_array_t), sitype_array);
  array->count = 0;
  array->alloc_size = alloc_size;
  array->data = (siheap_array_data_t *) siheap_malloc(sizeof(siheap_array_data_t) + alloc_size*sizeof(sinanbox_t), sitype_array_data);

  for (address_t i = 0; i < alloc_size; ++i) {
    array->data->data[i] = NANBOX_OFUNDEF();
  }

  return array;
)

SINTER_INLINEIFC sinanbox_t siarray_get(siheap_array_t *array, address_t index) SINTER_BODYIFC(
  if (index >= array->count) {
    return NANBOX_OFUNDEF();
  }

  return array->data->data[index];
)

SINTER_INLINEIFC void siarray_put(siheap_array_t *array, address_t index, sinanbox_t v) SINTER_BODYIFC(
  if (index >= array->alloc_size) {
    address_t new_size = array->alloc_size;
    while (new_size && new_size <= index) {
      new_size <<= 1;
    }
    if (!new_size) {
      new_size = UINT32_MAX;
    }
    array->data = (siheap_array_data_t *) siheap_mrealloc(&array->data->header,
      sizeof(siheap_array_data_t) + new_size*sizeof(sinanbox_t));
    array->alloc_size = new_size;
  }

  array->data->data[index] = v;
  if (array->count <= index) {
    array->count = index + 1;
  }
)

SINTER_INLINEIFC void siarray_destroy(siheap_array_t *array) SINTER_BODYIFC(
  for (address_t i = 0; i < array->alloc_size; ++i) {
    siheap_derefbox(array->data->data[i]);
  }
  siheap_deref(array->data);
)

#ifdef __cplusplus
}
#endif

#endif
