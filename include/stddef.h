#ifndef STDDEF_H
#define STDDEF_H

#include <stdint.h>

// NULL pointer
#ifndef NULL
#define NULL ((void*)0)
#endif

// offsetof macro
#define offsetof(type, member) ((size_t)&(((type*)0)->member))

// ptrdiff_t
typedef int32_t ptrdiff_t;

// size_t
typedef uint32_t size_t;

// wchar_t
typedef int32_t wchar_t;

#endif // STDDEF_H
