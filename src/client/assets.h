typedef struct
{
    s32 width;
    s32 height;
    union {
        uint32_t *pixels;
        uint8_t *alphas;
    };
} Bitmap;

typedef struct
{
    s32 samples_per_second;
    s32 sample_count;
    s16 *samples;
} Sound;

typedef struct
{
    s32 xoff;
    s32 yoff;
    s32 xadvance;
    Bitmap bitmap;
} Glyph;

typedef struct
{
    s32 baseline;
    s32 x_advance;
    s32 y_advance;
    Glyph glyphs[96];
} Font;
