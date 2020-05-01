#ifndef SINTER_DISPLAY_H
#define SINTER_DISPLAY_H

#include "config.h"
#include "nanbox.h"
#include "heap.h"
#include "heap_obj.h"
#include "../sinter.h"

#ifndef __cplusplus
#define SIVMFN_PRINTFN(v) (_Generic((v), \
  char *: sinter_printer_string, \
  const char *: sinter_printer_string, \
  float: sinter_printer_float, \
  int32_t: sinter_printer_integer))
#define SIVMFN_PRINT(v, is_error) do { \
  if (SIVMFN_PRINTFN(v)) SIVMFN_PRINTFN(v)((v), (is_error)); \
} while (0)
#endif

SINTER_INLINEIFC void sidisplay_strobj(siheap_header_t *obj, bool is_error);
#ifndef __cplusplus
SINTER_INLINEIFC void sidisplay_strobj(siheap_header_t *obj, _Bool is_error) {
  switch (obj->type) {
    case sitype_strconst:
      SIVMFN_PRINT((const char *) ((siheap_strconst_t *) obj)->string->data, is_error);
      break;
    case sitype_strpair: {
      siheap_strpair_t *pair = (siheap_strpair_t *) obj;
      sidisplay_strobj(pair->left, is_error);
      sidisplay_strobj(pair->right, is_error);
      break;
    }
    case sitype_string: {
      siheap_string_t *str = (siheap_string_t *) obj;
      SIVMFN_PRINT((const char *) str->string, is_error);
      break;
    }

    case sitype_intcont:
    case sitype_array_data:
    case sitype_empty:
    case sitype_frame:
    case sitype_free:
    case sitype_env:
    case sitype_array:
    case sitype_function:
      break;
  }
}
#endif

SINTER_INLINEIFC void sidisplay_nanbox(sinanbox_t v, bool is_error);
#ifndef __cplusplus
SINTER_INLINEIFC void sidisplay_nanbox(sinanbox_t v, bool is_error) {
  switch (NANBOX_GETTYPE(v)) {
    NANBOX_CASES_TINT
      SIVMFN_PRINT((int32_t) NANBOX_INT(v), is_error);
      break;
    case NANBOX_TBOOL:
      SIVMFN_PRINT(NANBOX_BOOL(v) ? "true" : "false", is_error);
      break;
    case NANBOX_TUNDEF:
      SIVMFN_PRINT("undefined", is_error);
      break;
    case NANBOX_TNULL:
      SIVMFN_PRINT("null", is_error);
      break;
    NANBOX_CASES_TPTR {
      siheap_header_t *obj = SIHEAP_NANBOXTOPTR(v);
      if (obj->flag_displayed) {
        SIVMFN_PRINT("...<circular>", is_error);
        return;
      }
      switch (obj->type) {
        case sitype_strconst:
        case sitype_strpair:
        case sitype_string:
          sidisplay_strobj(obj, is_error);
          break;
        case sitype_array: {
          siheap_array_t *a = (siheap_array_t *) obj;
          obj->flag_displayed = true; // mark the array so we don't recursively display it
          SIVMFN_PRINT("[", is_error);
          for (size_t i = 0; i < a->count; ++i) {
            if (i) {
              SIVMFN_PRINT(", ", is_error);
            }
            sidisplay_nanbox(a->data->data[i], is_error);
          }
          SIVMFN_PRINT("]", is_error);
          obj->flag_displayed = false;
          break;
        }
        case sitype_intcont:
          SIVMFN_PRINT("<function (internal continuation)>", is_error);
          break;
        case sitype_function:
          SIVMFN_PRINT("<function>", is_error);
          break;
        case sitype_array_data:
        case sitype_empty:
        case sitype_frame:
        case sitype_free:
        case sitype_env:
        default:
          SIBUGM("Unexpected object type\n");
          break;
      }
      break;
    }
    default:
      if (NANBOX_ISFLOAT(v)) {
        SIVMFN_PRINT(NANBOX_FLOAT(v), is_error);
      } else {
        SIBUGM("Unexpected type\n");
      }
      break;
  }
}
#endif

#endif
