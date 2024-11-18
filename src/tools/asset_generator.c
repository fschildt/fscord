// This tool embeds assets into the binary

// Todo: @PERFORMANCE for the generated header
// guarantee that all generated buffers are 4 byte aligned,
// because accessing non-aligned memory is slow

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


// from https://de.wikipedia.org/wiki/RIFF_WAVE
typedef struct {
    // riff header
    char riff_name[4];
    u32  file_size_minus_8;
    char wave_name[4];

    // format header
    char format_name[4];
    u32 format_len;
    u16 format_tag;
    u16 channels;
    u32 samples_per_second;
    u32 bytes_per_second;
    u16 block_align;
    u16 bits_per_sample;

    // data header
    char data_name[4];
    u32 data_len;
} Wav_Header;



internal void*
read_entire_file(const char *pathname)
{
    FILE *f = fopen(pathname, "rb");
    if (!f)
    {
        printf("%s: fopen() failed\n", pathname);
        return 0;
    }

    u64 file_size;

    fseek(f, 0, SEEK_END);
    file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void *buff = malloc(file_size);
    if (!buff)
    {
        printf("%s: malloc() failed\n", pathname);
        fclose(f);
        return 0;
    }

    u64 bytes_read = fread(buff, 1, file_size, f);
    if (bytes_read != file_size)
    {
        printf("%s: fread() failed\n", pathname);
        fclose(f);
        free(buff);
        return 0;
    }

    fclose(f);
    return buff;
}

internal void
write_entire_file(Memory_Arena *arena, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f)
    {
        printf("%s: fopen() failed\n", path);
        return;
    }

    size_t written = fwrite(arena->memory, 1, arena->size_used, f);
    if (written != arena->size_used)
    {
        printf("%s: fwrite failed\n", path);
    }

    printf("written %zu bytes to file %s\n", written, path);
    fclose(f);
}

internal void
generate_string(Memory_Arena *arena, const char *str)
{
    void *mem = memory_arena_push(arena, strlen(str));
    memcpy(mem, str, strlen(str));
}

internal void
generate_byte_values(Memory_Arena *arena, u8 *buffer, u32 size, b32 has_successor)
{
    char entry[16];
    for (u32 it = 0; it < size; it++)
    {
        b32 is_last = it == size-1;
        if (it % 4 == 0)
        {
            sprintf(entry, "\n\t");
            generate_string(arena, entry);
        }
        if (is_last)
        {
            if (has_successor)
            {
                sprintf(entry, "%3d,\n", buffer[it]);
            }
            else
            {
                sprintf(entry, "%3d\n", buffer[it]);
            }
        }
        else
        {
            sprintf(entry, "%3d,", buffer[it]);
        }
        generate_string(arena, entry);
    }
}

internal void
generate_font(Memory_Arena *arena, const char *name, const char *src_path, const char *dest_path)
{
    void *ttf = read_entire_file(src_path);
    if (!ttf) return;

    int font_size = 20;
    stbtt_fontinfo font_info;
    if (!stbtt_InitFont(&font_info, ttf, stbtt_GetFontOffsetForIndex(ttf, 0)))
    {
        printf("stbtt_InitFont failed\n");
        free(ttf);
        return;
    }


    // set font metadata (vmetrics)
    f32 scale = stbtt_ScaleForPixelHeight(&font_info, font_size);

    i32 baseline, ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);

    baseline = scale * -descent;
    ascent   = scale * ascent;
    descent  = scale * descent;
    line_gap = scale * line_gap;

    int x_advance_unscaled;
    stbtt_GetCodepointHMetrics(&font_info, 'f', &x_advance_unscaled, 0);


    // generate first line
    char first_line[256];
    sprintf(first_line, "global_var u8 %s[] = {", name);
    generate_string(arena, first_line);


    // set values
    Font font;
    font.baseline = baseline;
    font.x_advance = x_advance_unscaled * scale;
    font.y_advance = ascent - descent + scale * line_gap;

    // - set glyph metadata and generate glyph bitmaps
    u32 cnt_glyphs = '~' - ' ' + 1;
    u64 bitmap_memory_offset = 0;
    for (u32 it = 0; it < cnt_glyphs; it++)
    {
        Glyph *glyph = font.glyphs + it;
        char c = it + ' ';

        i32 width, height, xoff, yoff;
        u8 *mono_bitmap_flipped = stbtt_GetCodepointBitmap(&font_info, 0, stbtt_ScaleForPixelHeight(&font_info, font_size), c, &width, &height, &xoff, &yoff);
        if (!mono_bitmap_flipped && c != ' ')
        {
            printf("stbtt_GetCodepointBitmap for '%c' failed\n", c);
        }
        yoff = -(yoff + height);


        u8 *mono_bitmap_correct = malloc(width*height);
        u8 *dest = mono_bitmap_correct;
        for (u32 y = 0; y < height; y++)
        {
            u8 *src = mono_bitmap_flipped + (height-y-1)*width;
            for (u32 x = 0; x < width; x++)
            {
                *dest++ = *src++;
            }
        }


        i32 x_advance, left_side_bearing;
        stbtt_GetCodepointHMetrics(&font_info, c, &x_advance, &left_side_bearing);
        x_advance *= scale;
        left_side_bearing *= scale;


        u32 bitmap_size = width * height;
        glyph->xoff = left_side_bearing;
        glyph->yoff = yoff;
        glyph->xadvance = x_advance;
        glyph->bitmap.width  = width;
        glyph->bitmap.height = height;
        glyph->bitmap.alphas = (u8*)bitmap_memory_offset;

        generate_byte_values(arena, mono_bitmap_correct, bitmap_size, true);
        bitmap_memory_offset += bitmap_size;

        free(mono_bitmap_correct);
        stbtt_FreeBitmap(mono_bitmap_flipped, 0);
    }

    // generate last line
    generate_byte_values(arena, (u8*)&font, sizeof(font), true);
    generate_string(arena, "};");

    write_entire_file(arena, dest_path);

    free(ttf);
}

internal void
generate_sound(Memory_Arena *arena, const char *name, const char *src_path, const char *dest_path)
{
    // generate first line
    char first_line[256];
    sprintf(first_line, "global_var u8 %s[] = {", name);
    generate_string(arena, first_line);

    // read file
    void *wav_buff = read_entire_file(src_path);
    if (!wav_buff) return;

    // init sound struct
    Wav_Header *wav = wav_buff;
    void *wav_samples = wav_buff + 44;
    Sound sound = {};
    sound.samples_per_second = wav->samples_per_second;
    sound.sample_count = wav->data_len / sizeof(i16);
    sound.samples = (i16*)sizeof(sound);

    // generate buffers
    generate_byte_values(arena, (u8*)&sound, sizeof(Sound), true);
    generate_byte_values(arena, wav_samples, wav->data_len, true);

    // generate last line
    generate_string(arena, "};");

    // write file
    write_entire_file(arena, dest_path);

    free(wav_buff);
}

int main(int argc, char **argv)
{
    size_t arena_mem_size = MEBIBYTES(2);
    void *arena_mem = malloc(arena_mem_size);
    if (!arena_mem)
    {
        printf("malloc(%zu) failed\n", arena_mem_size);
        return EXIT_FAILURE;
    }

    Memory_Arena arena;
    memory_arena_init(&arena, arena_mem, arena_mem_size);

    generate_sound(&arena, "g_asset_sound_user_connected", "./data/client/sound_user_connected.wav", "./src/client/generated/asset_sound_user_connected.c");
    memory_arena_reset(&arena);
    generate_sound(&arena, "g_asset_sound_user_disconnected", "./data/client/sound_user_disconnected.wav", "./src/client/generated/asset_sound_user_disconnected.c");
    memory_arena_reset(&arena);
    generate_font(&arena, "g_asset_font", "./data/client/Inconsolata-Regular.ttf", "./src/client/generated/asset_font.c");

    return EXIT_SUCCESS;
}
