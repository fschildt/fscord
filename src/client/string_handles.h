#ifndef STRING_HANDLES_H
#define STRING_HANDLES_H

#include <basic/mem_arena.h>
#include <basic/string.h>

#ifdef STRING_HANDLES_INTERNAL
#define MAYBE_EXTERN 
#else
#define MAYBE_EXTERN extern
#endif

MAYBE_EXTERN String32 *g_empty_string32;
MAYBE_EXTERN String32 *g_login_username_hint;
MAYBE_EXTERN String32 *g_login_servername_hint;
MAYBE_EXTERN String32 *g_login_warning_username_invalid;
MAYBE_EXTERN String32 *g_login_warning_servername_invalid;
MAYBE_EXTERN String32 *g_login_warning_connecting;

void string_handles_create(MemArena *arena);
void string_handles_load_language();

#endif // STRING_HANDLES_H
