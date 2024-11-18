#include <client/asset_manager.h>
#include <client/generated/asset_font.c>
#include <client/generated/asset_sound_user_connected.c>
#include <client/generated/asset_sound_user_disconnected.c>

Font *
asset_manager_load_font() {
    Font *font = (Font*)(g_asset_font + sizeof(g_asset_font) - sizeof(Font));
    i32 glyph_count = '~' - ' ' + 1;
    for (i32 i = 0; i < glyph_count; i++)
    {
        Glyph *glyph = font->glyphs + i;
        glyph->bitmap.alphas = (void*)g_asset_font + (u64)(glyph->bitmap.alphas);
    }
    return font;
}

Sound *
asset_manager_load_sound(i32 id) {
    if (id == 0) {
        Sound *sound = (Sound*)(g_asset_sound_user_connected);
        sound->samples = (i16*)(sound + 1);
        return sound;
    } else {
        Sound *sound = (Sound*)(g_asset_sound_user_disconnected);
        sound->samples = (i16*)(sound + 1);
        return sound;
    }
}

