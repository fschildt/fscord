#ifndef STRING_HANDLES_H
#define STRING_HANDLES_H

#include <basic/mem_arena.h>
#include <basic/string32.h>

typedef enum {
    SH_EMPTY,
    SH_LOGIN_USERNAME_HINT,
    SH_LOGIN_SERVERNAME_HINT,
    SH_LOGIN_WARNING_USERNAME_INVALID,
    SH_LOGIN_WARNING_SERVERNAME_INVALID,
    SH_LOGIN_WARNING_CONNECTING,
    SH_COUNT
} String32Handle;

void string32_handles_create(MemArena *arena);
void string32_handles_load_language();
String32* string32_value(String32Handle handle);

#endif // STRING_HANDLES_H
