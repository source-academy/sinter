#include <stdlib.h>
#include <string.h>

#include <sinter.h>
#include <sinter/display.h>
#include <sinter/heap_obj.h>
#include <sinter/program.h>
#include <sinter/vm.h>

static sinanbox_t hello_world(uint8_t argc, sinanbox_t *argv) {
  (void)argc;
  (void)argv;
  return NANBOX_OFUNDEF();
}

static const sivmfnptr_t internals[] = {hello_world};
static const size_t internals_count = sizeof(internals) / sizeof(*internals);

void setup_internals(void) {
  sivmfn_vminternals = internals;
  sivmfn_vminternal_count = internals_count;
}
