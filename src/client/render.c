internal void
render_rectangle(Sys_Offscreen_Buffer *screen, V2 pos, V2 dim, V3 color)
{
    s32 xmin = round_f32_to_s32(pos.x);
    s32 ymin = round_f32_to_s32(pos.y);
    s32 xmax = round_f32_to_s32(pos.x + dim.x) - 1;
    s32 ymax = round_f32_to_s32(pos.y + dim.y) - 1;

    if (xmin < 0)
    {
        xmin = 0;
    }
    if (ymin < 0)
    {
        ymin = 0;
    }
    if (xmax >= screen->width)
    {
        xmax = screen->width - 1;
    }
    if (ymax >= screen->height)
    {
        ymax = screen->height - 1;
    }

    u8 rshift = screen->red_shift;
    u8 gshift = screen->green_shift;
    u8 bshift = screen->blue_shift;
    for (s32 y = ymin; y <= ymax; y++)
    {
        u32 *pixels = screen->pixels + y * screen->width + xmin;
        for (s32 x = xmin; x <= xmax; x++)
        {
            f32 r = color.r * 255.0;
            f32 g = color.g * 255.0;
            f32 b = color.b * 255.0;
            u32 val = (u32)r << rshift | (u32)g << gshift | (u32)b << bshift;
            *pixels++ = val;
        }
    }
}

internal void
render_mono_bitmap(Sys_Offscreen_Buffer *screen, Bitmap *bitmap, V2 pos, V3 color)
{
    s32 x0 = round_f32_to_s32(pos.x);
    s32 y0 = round_f32_to_s32(pos.y);
    s32 x1 = x0 + bitmap->width - 1;
    s32 y1 = y0 + bitmap->height - 1;

    // dx -> how much do we cut away from the left of the bitmap
    // dy -> how much do we cut away from the bottom of the bitmap

    s32 dx = 0;
    s32 dy = 0;

    if (x0 < 0)
    {
        dx = x0;
        x0 = 0;
    }
    if (y0 < 0)
    {
        dy = y0;
        y0 = 0;
    }
    if (x1 >= screen->width)
    {
        x1 = screen->width - 1;
    }
    if (y1 >= screen->height)
    {
        y1 = screen->height - 1;
    }


    u8 rshift = screen->red_shift;
    u8 gshift = screen->green_shift;
    u8 bshift = screen->blue_shift;

    u8 *alphas = bitmap->alphas + (-dy * bitmap->width) + (-dx);
    u32 *pixels = screen->pixels + y0 * screen->width + x0;
    for (s32 y = y0; y <= y1; y++)
    {
        u8 *alpha_row = alphas;
        u32 *pixel_row = pixels;
        for (s32 x = x0; x <= x1; x++)
        {
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

internal void
render_border(Sys_Offscreen_Buffer *screen, V2 pos, V2 dim, f32 size, V3 color)
{
    V2 _pos;
    V2 _dim;

    _dim = v2(dim.x, size);
    _pos = pos;
    render_rectangle(screen, _pos, _dim, color);

    _pos.y += dim.y - size;
    render_rectangle(screen, _pos, _dim, color);


    _dim = v2(size, dim.y);
    _pos = pos;
    render_rectangle(screen, _pos, _dim, color);

    _pos.x += dim.x - size;
    render_rectangle(screen, _pos, _dim, color);
}

internal void
render_text(Sys_Offscreen_Buffer *screen, Font *font, const char *str, int len, V2 pos)
{
    V3 color = v3(0, 0, 0);

    f32 glyph_x = pos.x;
    f32 glyph_y = pos.y + font->baseline;
    for (int i = 0; i < len; i++)
    {
        char c = str[i];
        Glyph *glyph = get_glyph(font, c);

        if (c != ' ')
        {
            Bitmap *mono_bitmap = &glyph->bitmap;

            V2 glyph_pos = v2(glyph_x + glyph->xoff, glyph_y + glyph->yoff);
            render_mono_bitmap(screen, mono_bitmap, glyph_pos, color);
        }

        glyph_x += glyph->xadvance;
    }
}
