/**
 * This file is here to provide an exported symbol for all functions
 * declared as inline, which is most functions.
 *
 * We do this by defining away SINTER_INLINE (which is defined as just
 * `inline` in inline.h), so that the function definitions become normal
 * non-qualified definitions, which make them have external linkage.
 */

#include <sinter/config.h>

#ifdef SINTER_INLINE
#undef SINTER_INLINE
#endif
#define SINTER_INLINE

#include <sinter/heap.h>
#include <sinter/heap_obj.h>
#include <sinter/nanbox.h>
#include <sinter/stack.h>
