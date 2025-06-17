#include <basic/basic.h>
#include <basic/mem_arena.h>
#include <client/string32_handles.h>


internal_var MemArena arena;
internal_var String32 **string32_values;


String32*
string32_value(String32Handle handle)
{
    if (handle < SH_COUNT) {
        return string32_values[handle];
    }
    else {
        return string32_values[SH_EMPTY];
    }
}


void
string32_handles_load_language(void)
{
    // Todo: read these from files.

    mem_arena_reset(&arena);

    string32_values = mem_arena_push(&arena, SH_COUNT*sizeof(*string32_values));
    string32_values[SH_EMPTY] = string32_create_from_ascii(&arena, "");
    string32_values[SH_LOGIN_USERNAME_HINT] = string32_create_from_ascii(&arena, "username:");
    string32_values[SH_LOGIN_SERVERNAME_HINT] = string32_create_from_ascii(&arena, "servername:");
    string32_values[SH_LOGIN_WARNING_USERNAME_INVALID] = string32_create_from_ascii(&arena, "error: username is invalid.");
    string32_values[SH_LOGIN_WARNING_SERVERNAME_INVALID] = string32_create_from_ascii(&arena, "error: servername is invalid.");
    string32_values[SH_LOGIN_WARNING_CONNECTING] = string32_create_from_ascii(&arena, "connecting...");
    string32_values[SH_LOGIN_WARNING_COULD_NOT_CONNECT] = string32_create_from_ascii(&arena, "error: could not connect");
    string32_values[SH_LOGIN_WARNING_CONNECTION_LOST] = string32_create_from_ascii(&arena, "error: connection lost");
}


void
string32_handles_create(MemArena *creator_arena)
{
    arena = mem_arena_make_subarena(creator_arena, KIBIBYTES(1));
}

