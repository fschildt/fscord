#include <SDL2/SDL_keycode.h>
#include <os/os.h>
#include <basic/string.h>
#include <string.h>

void
string32_move_cursor_left(String32 *str, size_t *cursor)
{
    if (*cursor > 0) {
        *cursor -= 1;
    }
}

void
string32_move_cursor_right(String32 *str, size_t *cursor)
{
    if (*cursor < str->len) {
        *cursor += 1;
    }
}

b32
string32_delete_left(String32 *str, size_t *cursor)
{
    return false;
}

b32
string32_delete_right(String32 *str, size_t *cursor)
{
    if (index >= 0) {
        size_t move_count = str->len - *cursor;
        memmove(&str->p[*cursor]-1, &str->p[*cursor], move_count);
        str->len--;
        return true;
    }
    return false;
}

b32
string32_insert(String32 *str, u32 codepoint, size_t *cursor)
{
    if (*cursor < str->len && *cursor < str->max_len) {
        size_t move_count = str->len - *cursor;
        memmove(&str->p[*cursor]+1, &str->p[*cursor], move_count);
        str->p[*cursor] = codepoint;
        str->len++;
        return true;
    }
    return false;
}

void
string32_edit(String32 *str, OSKeyPress key_press, size_t *cursor)
{
    if (key_press.is_unicode) {
        switch (key_press.code) {
        case '\b': {
            string32_delete_left(str, cursor);
        } break;

        case 127: {
            string32_delete_right(str, cursor);
        } break;

        default: {
            string32_insert(str, key_press.code, cursor);
        }
        }
    }
    else {
        if (key_press.code == OS_KEYCODE_LEFT) {
            string32_move_cursor_left(str, cursor);
        }
        else if (key_press.code == OS_KEYCODE_RIGHT) {
            string32_move_cursor_right(str, cursor);
        }
    }
}

b32
string32_equal(String32 *str1, String32 *str2)
{
    if (str1->len != str2->len) {
        return false;
    }

    for (size_t i = 0; i < str1->len; i++) {
        if (str1->p[i] != str2->p[i]) {
            return false;
        }
    }

    return true;
}

void
string32_copy_string32_with_count(String32 *dest, String32 *src, size_t count)
{
    assert(count <= dest->max_len);
    for (size_t i = 0; i < count; i++) {
        dest->p[i] = src->p[i];
    }
    dest->len = count;
}

void
string32_copy_string32(String32 *dest, String32 *src)
{
    assert(src->len <= dest->max_len);
    for (size_t i = 0; i < src->len; i++) {
        dest->p[i] = src->p[i];
    }
    dest->len = src->len;
}

void
string32_copy_ascii_cstr(String32 *dest, char *src)
{
    u32 *dest_str = dest->p;
    while (*src) {
        *dest_str++ = *src++;
    }
    dest->len = dest_str - dest->p;
}

void
string32_append_string32(String32 *dest, String32 *src)
{
    assert(dest->len + src->len <= dest->max_len);
    for (size_t i = 0; i < src->len; i++) {
        dest->p[dest->len + i] = src->p[i];
    }
    dest->len += src->len;
}

void
string32_append_ascii_cstr(String32 *dest, char *src)
{
    while (*src) {
        assert(dest->len < dest->max_len);
        dest->p[dest->len++] = *src;
    }
}

void
string32_reset(String32 *str)
{
    str->len = 0;
}

String32 *
string32_create(MemArena *arena, size_t max_len)
{
    String32 *str = mem_arena_push(arena, sizeof(String32));
    if (max_len > 1) {
        mem_arena_push(arena, (max_len-1)*sizeof(u32));
    }
    str->len = 0;
    str->max_len = max_len;
    return str;
}

