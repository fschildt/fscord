#ifndef FONT_H
#define FONT_H

#include <basic/basic.h>
#include <basic/string32.h>

typedef struct {
    i32 width;
    i32 height;
    union {
        uint32_t *pixels;
        uint8_t *alphas;
    };
} Bitmap;

typedef struct {
    i32 xoff;
    i32 yoff;
    i32 xadvance;
    Bitmap bitmap;
} Glyph;

typedef struct {
    i32 baseline;
    i32 x_advance;
    i32 y_advance;
    Glyph glyphs[96];
} Font;

Glyph*  font_get_glyph(Font *font, u32 codepoint);
f32     font_string32_width(Font *font, String32 *str);
size_t  font_string32_len_via_width(Font *font, f32 width);
f32     font_string32_width_via_len(Font *font, size_t len);
f32     font_string32_height(Font *font);

#endif // FONT_H
