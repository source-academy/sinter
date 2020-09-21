#include <sinter/vm.h>
#include <sinter/program.h>
#include <sinter/heap_obj.h>
#include <sinter/display.h>
#include <sinter.h>

void display_object_result(sinter_value_t *res, _Bool is_error) {
  if (res->type == sinter_type_array || res->type == sinter_type_function) {
    sinanbox_t arr = NANBOX_WITH_I32(res->object_value);
    sidisplay_nanbox(arr, is_error);
  }
}
