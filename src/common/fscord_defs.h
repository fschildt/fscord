#ifndef FSCORD_DEFS_H
#define FSCORD_DEFS_H


#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#define internal static

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef int8_t  b8;
typedef int16_t b16;
typedef int32_t b32;
typedef int64_t b64;

typedef float f32;
typedef double f64;

#define U8_MAX UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

#define S8_MAX INT8_MAX
#define S16_MAX INT16_MAX
#define S32_MAX INT32_MAX
#define S64_MAX INT64_MAX

#define F32_MAX FLT_MAX
#define F64_MAX DBL_MAX


#define KILOBYTES(x) (1000*x)
#define MEGABYTES(x) (1000*KILOBYTES(x))
#define GIGABYTES(x) (1000*MEGABYTES(x))

#define KIBIBYTES(x) (1024*x)
#define MEBIBYTES(x) (1024*KIBIBYTES(x))
#define GIBIBYTES(x) (1024*MEBIBYTES(x))

#define ARRAY_COUNT(array) sizeof(array) / sizeof(array[0])


#endif // FSCORD_DEFS_H
