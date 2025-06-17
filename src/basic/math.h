#ifndef MATH_H
#define MATH_H

#include <basic/basic.h>

typedef struct {
    f32 x, y;
} V2F32;

typedef struct {
    f32 x, y, z;
} V3F32;

typedef struct {
    f32 x, y, z, w;
} V4F32;

typedef struct {
    f32 x0;
    f32 y0;
    f32 x1;
    f32 y1;
} RectF32;

V2F32 v2f32(f32 x, f32 y);
V2F32 v2f32_add(V2F32 v1, V2F32 v2);
V2F32 v2f32_sub(V2F32 v1, V2F32 v2);
V2F32 v2f32_center(V2F32 dim, V2F32 dim_total);

V3F32 v3f32(f32 x, f32 y, f32 z);

V4F32 v4f32(f32 x, f32 y, f32 z, f32 w);

i32 f32_round_to_i32(f32 value);
f32 f32_center(f32 dim, f32 dim_total);

RectF32 rectf32(f32 x0, f32 y0, f32 x1, f32 y1);
b32     rectf32_contains_v2f32(RectF32 rect, V2F32 pos);



#endif // MATH_H
