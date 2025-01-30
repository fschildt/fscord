#include <os/os.h>
#include <basic/math.h>
#include <client/draw.h>

void draw_rectf32(OSOffscreenBuffer *offscreen, RectF32 rect, V3F32 color)
{
    i32 xmin = f32_round_to_i32(rect.x0);
    i32 ymin = f32_round_to_i32(rect.y0);
    i32 xmax = f32_round_to_i32(rect.x1);
    i32 ymax = f32_round_to_i32(rect.y1);

    if (xmin < 0) {
        xmin = 0;
    }
    if (ymin < 0) {
        ymin = 0;
    }
    if (xmax >= offscreen->width) {
        xmax = offscreen->width - 1;
    }
    if (ymax >= offscreen->height) {
        ymax = offscreen->height - 1;
    }

    u8 rshift = offscreen->red_shift;
    u8 gshift = offscreen->green_shift;
    u8 bshift = offscreen->blue_shift;
    for (i32 y = ymin; y <= ymax; y++) {
        u32 *pixel = offscreen->pixels + y * offscreen->width + xmin;
        for (i32 x = xmin; x <= xmax; x++) {
            f32 r = color.r * 255.0;
            f32 g = color.g * 255.0;
            f32 b = color.b * 255.0;
            u32 val = (u32)r << rshift | (u32)g << gshift | (u32)b << bshift;
            *pixel++ = val;
        }
    }
}

void draw_mono_bitmap(OSOffscreenBuffer *screen, V2F32 pos, Bitmap *bitmap, V3F32 color)
{
    i32 x0 = f32_round_to_i32(pos.x);
    i32 y0 = f32_round_to_i32(pos.y);
    i32 x1 = x0 + bitmap->width - 1;
    i32 y1 = y0 + bitmap->height - 1;

    i32 cut_left = 0;
    i32 cut_bot = 0;

    if (x0 < 0) {
        cut_left = x0;
        x0 = 0;
    }
    if (y0 < 0) {
        cut_bot = y0;
        y0 = 0;
    }
    if (x1 >= screen->width) {
        x1 = screen->width - 1;
    }
    if (y1 >= screen->height) {
        y1 = screen->height - 1;
    }


    u8 rshift = screen->red_shift;
    u8 gshift = screen->green_shift;
    u8 bshift = screen->blue_shift;

    u8 *alphas = bitmap->alphas + (-cut_bot * bitmap->width) + (-cut_left);
    u32 *pixels = screen->pixels + y0 * screen->width + x0;
    for (i32 y = y0; y <= y1; y++) {
        u8 *alpha_row = alphas;
        u32 *pixel_row = pixels;
        for (i32 x = x0; x <= x1; x++) {
            f32 alpha = *alpha_row++ / 255.0;

            u32 pixel = *pixel_row;
            f32 r0 = (pixel >> rshift) & 0xff;
            f32 g0 = (pixel >> gshift) & 0xff;
            f32 b0 = (pixel >> bshift) & 0xff;

            f32 r1 = r0 + (color.r - r0)*alpha;
            f32 g1 = g0 + (color.g - g0)*alpha;
            f32 b1 = b0 + (color.b - b0)*alpha;

            pixel = (u32)b1 << bshift | (u32)g1 << gshift | (u32)r1 << rshift;
            *pixel_row++ = pixel;
        }
        alphas += bitmap->width;
        pixels += screen->width;
    }
}

void draw_border(OSOffscreenBuffer *screen, RectF32 rect, f32 size, V3F32 color)
{
    RectF32 draw_rect;

    // left
    draw_rect.x0 = rect.x0;
    draw_rect.y0 = rect.y0;
    draw_rect.x1 = rect.x0 + size;
    draw_rect.y1 = rect.y1;
    draw_rectf32(screen, draw_rect, color);

    // bottom
    draw_rect.x0 = rect.x0;
    draw_rect.y0 = rect.y0;
    draw_rect.x1 = rect.x1;
    draw_rect.y1 = rect.y0 + size;
    draw_rectf32(screen, draw_rect, color);

    // top
    draw_rect.x0 = rect.x0;
    draw_rect.y0 = rect.y1 - size;
    draw_rect.x1 = rect.x1;
    draw_rect.y1 = rect.y1;
    draw_rectf32(screen, draw_rect, color);

    // right
    draw_rect.x0 = rect.x1 - size;
    draw_rect.y0 = rect.y0;
    draw_rect.x1 = rect.x1;
    draw_rect.y1 = rect.y1;
    draw_rectf32(screen, draw_rect, color);
}

void draw_string32(OSOffscreenBuffer *screen, V2F32 pos, String32 *str, Font *font) {
    V3F32 color = v3f32(0, 0, 0);
    f32 glyph_x = pos.x;
    f32 glyph_y = pos.y + font->baseline;

    for (int i = 0; i < str->len; i++) {
        u32 codepoint = str->codepoints[i];
        Glyph *glyph = font_get_glyph(font, codepoint);

        if (codepoint != ' ') {
            Bitmap *mono_bitmap = &glyph->bitmap;
            V2F32 glyph_pos = v2f32(glyph_x + glyph->xoff, glyph_y + glyph->yoff);
            draw_mono_bitmap(screen, glyph_pos, mono_bitmap, color);
        }

        glyph_x += glyph->xadvance;
    }
}

