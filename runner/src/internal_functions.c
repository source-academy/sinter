#include <stdlib.h>
#include <string.h>

#include <sinter/vm.h>
#include <sinter/program.h>
#include <sinter/heap_obj.h>
#include <sinter.h>

static svm_constant_t *hello_world_string = NULL;

static sinanbox_t hello_world(uint8_t argc, sinanbox_t *argv) {
  (void) argc; (void) argv;
  siheap_strconst_t *str = sistrconst_new(hello_world_string);
  return SIHEAP_PTRTONANBOX(str);
}

static const sivmfnptr_t internals[] = { hello_world };
static const size_t internals_count = sizeof(internals)/sizeof(*internals);

void setup_internals(void) {
  static const char *hello_world_cstr = "Hello world!";
  hello_world_string = malloc(sizeof(svm_constant_t) + strlen(hello_world_cstr) + 1);
  hello_world_string->type = 1;
  hello_world_string->length = strlen(hello_world_cstr) + 1;
  strcpy((char *) hello_world_string->data, hello_world_cstr);

  sivmfn_vminternals = internals;
  sivmfn_vminternal_count = internals_count;
}
