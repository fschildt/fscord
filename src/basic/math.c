#include <basic/basic.h>
#include <basic/math.h>

i32
f32_round_to_i32(f32 value)
{
    i32 result;
    if (value >= 0) {
        result = (i32)(value + 0.5f);
    } else {
        result = (i32)(value - 0.5f);
    }
    return result;
}

f32
f32_center(f32 dim, f32 dim_total)
{
    f32 result = (dim_total - dim) / 2;
    return result;
}

V2F32
v2f32(f32 x, f32 y)
{
    V2F32 result = {x, y};
    return result;
}

V2F32
v2f32_add(V2F32 v1, V2F32 v2)
{
    V2F32 result = {v1.x + v2.x, v1.y + v2.y};
    return result;
}

V2F32
v2f32_sub(V2F32 v1, V2F32 v2)
{
    V2F32 result = {v1.x - v2.x, v1.y - v2.y};
    return result;
}

V2F32
v2f32_center(V2F32 dim_inner, V2F32 dim_outer)
{
    V2F32 pos;
    pos.x = f32_center(dim_inner.x, dim_outer.x);
    pos.y = f32_center(dim_inner.y, dim_outer.y);
    return pos;
}

V3F32
v3f32(f32 x, f32 y, f32 z)
{
    V3F32 result = {x, y, z};
    return result;
}

V4F32
v4f32(f32 x, f32 y, f32 z, f32 w)
{
    V4F32 result = {x, y, z, w};
    return result;
}

RectF32
rectf32(float x0, float y0, float x1, float y1)
{
    RectF32 result = {x0, y0, x1, y1};
    return result;
}

b32
rectf32_contains_v2f32(RectF32 rect, V2F32 pos)
{
    b32 result = pos.x >= rect.x0 &&
                 pos.x <= rect.x1 &&
                 pos.y >= rect.y0 &&
                 pos.y <= rect.y1;
    return result;
}


