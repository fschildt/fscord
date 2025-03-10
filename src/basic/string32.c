#include "basic/mem_arena.h"
#include <SDL2/SDL_keycode.h>
#include <os/os.h>
#include <string.h>

#include <basic/basic.h>
#include <basic/string32.h>



void
string32_buffer_move_cursor_left(String32Buffer *buffer)
{
    if (buffer->cursor > 0) {
        buffer->cursor -= 1;
    }
}


void
string32_buffer_move_cursor_right(String32Buffer *buffer)
{
    if (buffer->cursor < buffer->len) {
        buffer->cursor += 1;
    }
}


b32
string32_buffer_del_left(String32Buffer *buffer)
{
    if (buffer->cursor > 0){
        u32 *dest = &buffer->codepoints[buffer->cursor - 1];
        u32 *src  = &buffer->codepoints[buffer->cursor];
        size_t move_size = (buffer->len - buffer->cursor) * sizeof(u32);
        memmove(dest, src, move_size);

        buffer->len -= 1;
        buffer->cursor -= 1;

        return true;
    }

    return false;
}


b32
string32_buffer_del_right(String32Buffer *buffer)
{
    if (buffer->cursor < buffer->len) {
        u32 *dest = &buffer->codepoints[buffer->cursor];
        u32 *src  = &buffer->codepoints[buffer->cursor + 1];
        size_t move_size = (buffer->len - buffer->cursor) * sizeof(u32);
        memmove(dest, src, move_size);

        buffer->len -= 1;

        return true;
    }

    return false;
}


b32
string32_buffer_insert(String32Buffer *buffer, u32 codepoint)
{
    if (buffer->len < buffer->max_len &&
        buffer->cursor <= buffer->len)
    {
        u32 *dest = &buffer->codepoints[buffer->cursor + 1];
        u32 *src  = &buffer->codepoints[buffer->cursor];
        size_t move_size = (buffer->len - buffer->cursor) * sizeof(u32);
        memmove(dest, src, move_size);

        buffer->codepoints[buffer->cursor] = codepoint;
        buffer->len += 1;
        buffer->cursor += 1;

        return true;
    }

    return false;
}


void
string32_buffer_edit(String32Buffer *buffer, OSKeyPress key_press)
{
    if (key_press.is_unicode) {
        switch (key_press.code) {
        case '\b': {
            string32_buffer_del_left(buffer);
        } break;

        case 127: {
            string32_buffer_del_right(buffer);
        } break;

        default: {
            string32_buffer_insert(buffer, key_press.code);
        }
        }
    }
    else {
        if (key_press.code == OS_KEYCODE_LEFT) {
            string32_buffer_move_cursor_left(buffer);
        }
        else if (key_press.code == OS_KEYCODE_RIGHT) {
            string32_buffer_move_cursor_right(buffer);
        }
    }
}


void
string32_buffer_append_string32_buffer(String32Buffer *buffer, String32Buffer *src_buffer)
{
    size_t len_avail = buffer->max_len - buffer->len;
    size_t copy_count = len_avail >= src_buffer->len ? src_buffer->len : len_avail;
    u32 *dest = buffer->codepoints + buffer->len;
    u32 *src = src_buffer->codepoints;
    for (size_t i = 0; i < copy_count; i++) {
        *dest++ = *src++;
    }
    buffer->len += copy_count;
}


void
string32_buffer_append_string32(String32Buffer *buffer, String32 *str)
{
    size_t len_avail = buffer->max_len - buffer->len;
    size_t copy_count = str->len >= len_avail ? len_avail : str->len;
    u32 *dest = buffer->codepoints + buffer->len;
    u32 *src = str->codepoints;
    for (size_t i = 0; i < copy_count; i++) {
        *dest++ = *src++;
    }
    buffer->len += copy_count;
}


void
string32_buffer_append_ascii_cstr(String32Buffer *buffer, char *ascii)
{
    while (*ascii) {
        if (buffer->len >= buffer->max_len) {
            break;
        }

        buffer->codepoints[buffer->len] = *ascii;
        buffer->len += 1;
        ascii++;
    }
}


void
string32_buffer_copy_string32_buffer(String32Buffer *dest, String32Buffer *src)
{
    string32_buffer_reset(dest);
    string32_buffer_append_string32_buffer(dest, src);
}


void
string32_buffer_copy_string32(String32Buffer *buffer, String32 *str)
{
    string32_buffer_reset(buffer);
    string32_buffer_append_string32(buffer, str);
}


b32 string32_buffer_equal_string32(String32Buffer *buffer, String32 *str)
{
    if (buffer->len != str->len) {
        return false;
    }
    for (size_t i = 0; i < buffer->len; i++) {
        if (buffer->codepoints[i] != str->codepoints[i]) {
            return false;
        }
    }
    return true;
}


String32 *
string32_buffer_to_string32_with_len(MemArena *arena, String32Buffer *buffer, size_t len)
{
    size_t push_size = sizeof(String32) + len * sizeof(u32);
    String32 *result = mem_arena_push(arena, push_size);

    memcpy(result->codepoints, buffer->codepoints, len * sizeof(u32));
    result->len = len;

    return result;
}


String32 *
string32_buffer_to_string32(MemArena *arena, String32Buffer *buffer)
{
    size_t push_size = sizeof(String32) + buffer->len * sizeof(u32);
    String32 *result = mem_arena_push(arena, push_size);

    memcpy(result->codepoints, buffer->codepoints, buffer->len * sizeof(u32));
    result->len = buffer->len;

    return result;
}


void
string32_buffer_print(String32Buffer *buffer)
{
    for (size_t i = 0; i < buffer->len; i++) {
        putchar(buffer->codepoints[i]);
    }
}


void
string32_buffer_reset(String32Buffer *buffer)
{
    buffer->len = 0;
    buffer->cursor = 0;
}

void
string32_buffer_init_in_place(String32Buffer *buffer, size_t max_len)
{
    buffer->cursor = 0;
    buffer->len = 0;
    buffer->max_len = max_len;
}


String32Buffer *
string32_buffer_create(MemArena *arena, size_t max_len)
{
    size_t push_size = sizeof(String32Buffer) + max_len * sizeof(u32);
    String32Buffer *buffer = mem_arena_push(arena, push_size);

    buffer->cursor = 0;
    buffer->len = 0;
    buffer->max_len = max_len;

    return buffer;
}


void
string32_print(String32 *str)
{
    for (size_t i = 0; i < str->len; i++) {
        putchar(str->codepoints[i]);
    }
}


b32
string32_equal(String32 *str1, String32 *str2) { if (str1->len != str2->len) {
        return false;
    }

    for (size_t i = 0; i < str1->len; i++) {
        if (str1->codepoints[i] != str2->codepoints[i]) {
            return false;
        }
    }

    return true;
}


String32 *
string32_create_from_u32_array(MemArena *arena, u32 *buffer, size_t len)
{
    size_t push_size = sizeof(String32) + len * sizeof(u32);
    String32 *result = mem_arena_push(arena, sizeof(String32));

    memcpy(result->codepoints, buffer, len);
    result->len = len;

    return result;
}



String32 *
string32_create_from_string32(MemArena *arena, String32 *src)
{
    size_t push_size = sizeof(String32) + src->len * sizeof(u32);
    String32 *result = mem_arena_push(arena, push_size);

    for (size_t i = 0; i < src->len; i++) {
        result->codepoints[i] = src->codepoints[i];
    }
    result->len = src->len;

    return result;
}


String32 *
string32_create_from_ascii(MemArena *arena, char *ascii)
{
    size_t len = strlen(ascii);

    size_t push_size = sizeof(String32) + strlen(ascii) * sizeof(u32);
    String32 *result = mem_arena_push(arena, push_size);

    u32 *dest = result->codepoints;
    while (*ascii) {
        *dest++ = *ascii++;
    }
    result->len = len;

    return result;
}

