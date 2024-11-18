#ifndef STRING_H
#define STRING_H

#include <basic/basic.h>
#include <basic/mem_arena.h>
#include <os/os.h>

typedef struct {
    u32 len;
    u32 max_len;
    u32 p[1]; // additional codepoints follow in memory
} String32;

String32 *string32_create(MemArena *arena, size_t max_len);
void      string32_reset(String32 *str);
void      string32_copy_string32(String32 *dest, String32 *src);
void      string32_copy_string32_with_count(String32 *dest, String32 *src, size_t count);
void      string32_copy_ascii_cstr(String32 *dest, char *src);
void      string32_append_ascii_cstr(String32 *dest, char *src);
void      string32_append_string32(String32 *dest, String32 *src);
b32       string32_equal(String32 *str1, String32 *str2);

void      string32_edit(String32 *str, OSKeyPress key_press, size_t *cursor);
b32       string32_insert(String32 *str, u32 codepoint, size_t *cursor);
b32       string32_delete_right(String32 *str, size_t *cursor);
b32       string32_delete_left(String32 *str, size_t *cursor);
void      string32_move_cursor_left(String32 *str, size_t *cursor);
void      string32_move_cursor_right(String32 *str, size_t *cursor);

#endif // STRING_H
