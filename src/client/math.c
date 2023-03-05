internal V2
v2(f32 x, f32 y)
{
    V2 result = {};
    result.x = x;
    result.y = y;
    return result;
}

internal V3
v3(f32 x, f32 y, f32 z)
{
    V3 result = {};
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

internal V2
v2_add(V2 a, V2 b)
{
    V2 result = {};
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

internal s32
round_f32_to_s32(f32 value)
{
    s32 result;
    if (value >= 0)
    {
        result = (s32)(value + 0.5f);
    }
    else
    {
        result = (s32)(value - 0.5f);
    }
    return result;
}

internal f32
center_f32(f32 dim, f32 dim_total)
{
    f32 result = (dim_total - dim) / 2;
    return result;
}

internal V2
center_v2(V2 dim, V2 dim_total)
{
    V2 pos = {};
    pos.x = center_f32(dim.x, dim_total.x);
    pos.y = center_f32(dim.y, dim_total.y);
    return pos;
}

