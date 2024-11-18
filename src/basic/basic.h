#ifndef BASIC_H
#define BASIC_H

#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint8_t b8;
typedef uint16_t b16;
typedef uint32_t b32;
typedef uint64_t b64;

typedef float f32;
typedef double f64;

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

#define KIBIBYTES(x) ((x)*1024)
#define MEBIBYTES(x) ((x)*KIBIBYTES(1024))
#define GIBIBYTES(x) ((x)*MEBIBYTES(1024))

#define I8_MAX INT8_MAX
#define I16_MAX INT16_MAX
#define I32_MAX INT32_MAX
#define I64_MAX INT64_MAX

#define U8_MAX UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

#define F32_MAX FLT_MAX
#define F64_MAX DBL_MAX
#define F32_MIN FLT_MIN
#define F64_MIN DBL_MIN

#define likely(expr) __builtin_expect(!!(expr), 1)
#define unlikely(expr) __builtin_expect(!!(expr), 0)

#define InvalidCodePath assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break

#define internal_var static
#define internal_fn static
#define persist_var static

#ifdef NDEBUG
#define debug_printf()
#else
#define debug_printf(...) printf(__VA_ARGS__)
#endif


#if defined(__linux__)
#include <alloca.h>
#elif defined(_WIN64)
#define alloca(size) _alloca(size)
#endif

// other compiler definitions that should not be forgotten:
// __LINE__
// __FILE__

#endif // BASIC_H
