#ifndef SINTER_CONFIG_H
#define SINTER_CONFIG_H

#ifndef SINTER_DEBUG_LOGLEVEL
#define SINTER_DEBUG_LOGLEVEL 0
#endif

#ifdef SINTER_STATIC_HEAP
#ifndef SINTER_HEAP_SIZE
// "64 KB ought to be enough for anybody"
#define SINTER_HEAP_SIZE 0x10000
#endif
#else
#define SINTER_HEAP_SIZE (siheap_size)
#endif

#ifndef SINTER_STACK_ENTRIES
#define SINTER_STACK_ENTRIES 0x200
#endif

#ifndef SINTER_INLINE
#define SINTER_INLINE inline
#endif

#ifdef __cplusplus
#define _Static_assert static_assert
#define _Noreturn [[noreturn]]
#define _Bool bool
#define SINTER_BODYIFC(...) ;
#define SINTER_INLINEIFC
#else
#define SINTER_INLINEIFC SINTER_INLINE
/**
 * Results in its arguments wrapped in braces if in C, or a semicolon in C++.
 *
 * Used to hide function bodies from C++.
 */
#define SINTER_BODYIFC(...) { __VA_ARGS__ }
#endif

#ifdef SINTER_SEATBELTS_OFF
#define SINTER_BODYIFSB(...)
#else
/**
 * Results in its arguments if SINTER_SEATBELTS_OFF is not defined,
 * or nothing if it is.
 *
 * Used in SINTER_BODYIFC blocks to disable safety-checking code when SINTER_SEATBELTS_OFF
 * is defined.
 */
#define SINTER_BODYIFSB(...) __VA_ARGS__
#endif

#endif
