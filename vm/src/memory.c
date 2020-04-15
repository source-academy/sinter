#include <string.h>

#include <sinter/config.h>
#include <sinter/heap.h>
#include <sinter/heap_obj.h>
#include <sinter/stack.h>

#ifdef SINTER_STATIC_HEAP
unsigned char siheap[SINTER_HEAP_SIZE] = { 0 };
#else
unsigned char *siheap = NULL;
size_t siheap_size = 0;
#endif

siheap_free_t *siheap_first_free = NULL;

sinanbox_t sistack[SINTER_STACK_ENTRIES] = { 0 };

sinanbox_t *sistack_bottom = sistack;
sinanbox_t *sistack_limit = sistack;
sinanbox_t *sistack_top = sistack;

/**
 * Runs the destructor for the given heap object.
 *
 * The destructor typically just decrements the reference counts
 * of objects referred to by the given object.
 */
void siheap_mdestroy(siheap_header_t *ent) {
  switch (ent->type) {
  case sitype_env:
    sienv_destroy((siheap_env_t *) ent);
    break;
  case sitype_strpair:
    sistrpair_destroy((siheap_strpair_t *) ent);
    break;
  case sinter_type_function:
    sifunction_destroy((siheap_function_t *) ent);
    break;
  }
}

void sistack_init(void) {
  sistack_bottom = sistack;
  sistack_limit = sistack;
  sistack_top = sistack;
}

static address_t sizeof_strobj(siheap_header_t *obj) {
  switch (obj->type) {
  case sitype_strconst: {
    siheap_strconst_t *v = (siheap_strconst_t *) obj;
    return v->string->length - 1;
  }

  case sitype_strpair: {
    siheap_strpair_t *v = (siheap_strpair_t *) obj;
    return sizeof_strobj(v->left) + sizeof_strobj(v->right);
  }

  case sitype_string: {
    siheap_string_t *v = (siheap_string_t *) obj;
    return v->header.size - sizeof(siheap_string_t) - 1;
  }

  default:
    SIBUGM("Unknown string type\n");
    sifault(sinter_fault_internal_error);
    break;
  }
}

static void write_strobj(siheap_header_t *obj, char **to) {
  switch (obj->type) {
  case sitype_strconst: {
    siheap_strconst_t *v = (siheap_strconst_t *) obj;
    memcpy(*to, v->string->data, v->string->length - 1);
    *to += v->string->length - 1;
    return;
  }

  case sitype_strpair: {
    siheap_strpair_t *v = (siheap_strpair_t *) obj;
    write_strobj(v->left, to);
    write_strobj(v->right, to);
    return;
  }

  case sitype_string: {
    siheap_string_t *v = (siheap_string_t *) obj;
    const address_t size = v->header.size - sizeof(siheap_string_t) - 1;
    memcpy(*to, v->string, size);
    *to += size;
    return;
  }

  default:
    SIBUGM("Unknown string type\n");
    sifault(sinter_fault_internal_error);
    break;
  }
}

siheap_string_t *sistrpair_flatten(siheap_strpair_t *obj) {
  if (!obj->right) {
    return (siheap_string_t *) obj->left;
  }

  address_t strsize = sizeof_strobj(&obj->header);
  siheap_string_t *string = sistring_new(strsize + 1);
  char *to = string->string;
  write_strobj(&obj->header, &to);
  to[strsize] = '\0';

  siheap_ref(string);
  obj->left = &string->header;
  obj->right = NULL;
  return string;
}
