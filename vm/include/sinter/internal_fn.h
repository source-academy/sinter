#ifndef SINTER_INTERNAL_FN_H
#define SINTER_INTERNAL_FN_H

#include "config.h"

#include <stdint.h>

#include "nanbox.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a VM-internal function.
 *
 * A VM-internal function should not modify the stack.
 */
typedef sinanbox_t (*sivmfnptr_t)(uint8_t argc, sinanbox_t *argv);

extern sivmfnptr_t sivmfn_primitives[];
#define SIVMFN_PRIMITIVE_COUNT (92)

extern const sivmfnptr_t *sivmfn_vminternals;
extern size_t sivmfn_vminternal_count;

#ifdef __cplusplus
}
#endif

#endif
