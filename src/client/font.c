#include <client/font.h>
#include <assert.h>

Glyph *
font_get_glyph(Font *font, u32 codepoint)
{
    assert(codepoint >= ' ' && codepoint <= '~');
    i32 index = codepoint - ' ';
    Glyph *glyph = &font->glyphs[index];
    return glyph;
}

f32
font_get_width_from_string32(Font *font, String32 *str)
{
    f32 width = font->x_advance * str->len;
    return width;
}

f32
font_get_width_from_string32_len(Font *font, size_t len)
{
    f32 width = font->x_advance * len;
    return width;
}

size_t
font_get_string32_len_from_width(Font *font, f32 width)
{
    size_t len = font->x_advance / width;
    return len;
}

f32
font_get_height_from_string32(Font *font)
{
    return font->y_advance;
}
