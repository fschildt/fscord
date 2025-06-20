#ifndef STRING32_H
#define STRING32_H

#include <basic/basic.h>
#include <basic/mem_arena.h>
#include <os/os.h>



typedef struct {
    u32 len;
    u32 codepoints[];
} String32;


typedef struct {
    u32 cursor;
    u32 len;
    u32 max_len;
    u32 codepoints[];
} String32Buffer;



String32 *string32_create_from_ascii(MemArena *arena, char *ascii);
String32 *string32_create_from_string32(MemArena *arena, String32 *src);
String32 *string32_create_from_string32_with_len(MemArena *arena, String32 *src, size_t len);
String32 *string32_create_from_u32_array(MemArena *arena, u32 *buffer, size_t len);

b32       string32_equal(String32 *str1, String32 *str2);
void      string32_print(String32 *str);



String32Buffer* string32_buffer_create(MemArena *arena, size_t max_len);
void            string32_buffer_init_in_place(String32Buffer *buffer, size_t max_len);

String32 *string32_buffer_to_string32(MemArena *arena, String32Buffer *buffer);
String32 *string32_buffer_to_string32_with_len(MemArena *arena, String32Buffer *buffer, size_t len);

void    string32_buffer_reset(String32Buffer *buffer);
void    string32_buffer_print(String32Buffer *buffer);

void    string32_buffer_append_ascii_cstr(String32Buffer *buffer, char *ascii);
void    string32_buffer_append_string32(String32Buffer *buffer, String32 *str);
void    string32_buffer_append_string32_buffer(String32Buffer *buffer, String32Buffer *src);

b32     string32_buffer_equal_string32(String32Buffer *buffer, String32 *str);
void    string32_buffer_copy_string32(String32Buffer *buffer, String32 *str);
void    string32_buffer_copy_string32_buffer(String32Buffer *dest, String32Buffer *src);

void    string32_buffer_edit(String32Buffer *buffer, OSEventKeyPress key_press);
b32     string32_buffer_insert(String32Buffer *buffer, u32 codepoint);
b32     string32_buffer_delete_right(String32Buffer *buffer);
b32     string32_buffer_delete_left(String32Buffer *buffer);
void    string32_buffer_move_cursor_left(String32Buffer *buffer);
void    string32_buffer_move_cursor_right(String32Buffer *buffer);


#endif // STRING32_H
