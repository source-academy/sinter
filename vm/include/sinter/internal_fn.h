#ifndef SINTER_INTERNAL_FN_H
#define SINTER_INTERNAL_FN_H

#include "config.h"

#include <stdint.h>

#include "nanbox.h"
#include "../sinter.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a VM-internal function.
 *
 * A VM-internal function should not modify the stack.
 */
typedef sinanbox_t (*sivmfnptr_t)(uint8_t argc, sinanbox_t *argv);

extern const sivmfnptr_t sivmfn_primitives[];
#define SIVMFN_PRIMITIVE_COUNT (92)

extern const sivmfnptr_t *sivmfn_vminternals;
extern size_t sivmfn_vminternal_count;

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

#ifdef __cplusplus
}
#endif

#endif
