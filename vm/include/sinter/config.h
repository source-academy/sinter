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
#define SINTER_INLINEIFC
#else
#define SINTER_INLINEIFC SINTER_INLINE
#endif

#if defined(SINTER_DEBUG) && defined(NDEBUG)
#undef NDEBUG
#endif

#endif
