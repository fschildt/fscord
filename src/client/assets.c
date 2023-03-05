internal Glyph *
get_glyph(Font *font, char c)
{
    assert(c >= ' ' && c <= '~');
    s32 index = c - ' ';
    Glyph *glyph = &font->glyphs[index];
    return glyph;
}

internal Font *
locate_font()
{
    Font *font = (Font*)(g_asset_font + sizeof(g_asset_font) - sizeof(Font));
    s32 glyph_count = '~' - ' ' + 1;
    for (s32 i = 0; i < glyph_count; i++)
    {
        Glyph *glyph = font->glyphs + i;
        glyph->bitmap.alphas = (void*)g_asset_font + (u64)(glyph->bitmap.alphas);
    }
    return font;
}

internal Sound *
locate_sound_user_connected()
{
    Sound *sound = (Sound*)(g_asset_sound_user_connected);
    sound->samples = (s16*)(sound + 1);
    return sound;
}

internal Sound *
locate_sound_user_disconnected()
{
    Sound *sound = (Sound*)(g_asset_sound_user_disconnected);
    sound->samples = (s16*)(sound + 1);
    return sound;
}
