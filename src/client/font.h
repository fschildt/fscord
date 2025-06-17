#ifndef FONT_H
#define FONT_H

#include <basic/basic.h>
#include <basic/string32.h>

typedef struct {
    i32 width;
    i32 height;
    union {
        uint32_t *rgba;
        uint8_t *grayscale;
    } data;
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
f32     font_get_width_from_string32(Font *font, String32 *str);
f32     font_get_width_from_string32_len(Font *font, size_t len);
f32     font_get_height_from_string32(Font *font);
size_t  font_get_string32_len_from_width(Font *font, f32 width);

#endif // FONT_H
