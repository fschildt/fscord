#define STRING_HANDLES_INTERNAL
#include <client/string_handles.h>

void string_handles_load_language()
{
    string32_copy_ascii_cstr(g_login_username_hint, "username:");
    string32_copy_ascii_cstr(g_login_servername_hint, "servername:");
    string32_copy_ascii_cstr(g_login_warning_username_invalid, "error: username is invalid.");
    string32_copy_ascii_cstr(g_login_warning_servername_invalid, "error: servername is invalid.");
    string32_copy_ascii_cstr(g_login_warning_connecting, "connecting...");
}

void string_handles_create(MemArena *arena)
{
    g_empty_string32 = string32_create(arena, 0);
    g_login_username_hint = string32_create(arena, 16);
    g_login_servername_hint = string32_create(arena, 16);
    g_login_warning_username_invalid = string32_create(arena, 32);
    g_login_warning_servername_invalid = string32_create(arena, 32);
    g_login_warning_connecting = string32_create(arena, 32);
}

