#include "basic/mem_arena.h"
#include <os/os.h>
#include <basic/string32.h>
#include <messages//messages.h>

#include <string.h>


internal_var OSNetSecureStream *s_secure_stream;
internal_var MemArena s_arena;


void
c2s_chat_message(String32 *content)
{
    C2S_ChatMessage *chat_message = mem_arena_push(&s_arena, sizeof(C2S_ChatMessage));

    String32 *content_copy = string32_create_from_string32(&s_arena, content);
    chat_message->content = (String32*)((u8*)content_copy - s_arena.size_used);

    chat_message->header.type = C2S_CHAT_MESSAGE;
    chat_message->header.size = s_arena.size_used;

    os_net_secure_stream_send(s_secure_stream, s_arena.memory, s_arena.size_used);
    mem_arena_reset(&s_arena);
}


void
c2s_login(String32 *username, String32 *password)
{
    C2S_Login *login = mem_arena_push(&s_arena, sizeof(C2S_Login));

    String32 *username_copy = string32_create_from_string32(&s_arena, username);
    login->username = (String32*)((u8*)username_copy - s_arena.size_used);

    String32 *password_copy = string32_create_from_string32(&s_arena, password);
    login->password = (String32*)((u8*)password_copy - s_arena.size_used);

    login->header.type = C2S_LOGIN;
    login->header.size = s_arena.size_used;

    os_net_secure_stream_send(s_secure_stream, s_arena.memory, s_arena.size_used);
    mem_arena_reset(&s_arena);
}


void
s2c_chat_message(String32 *username, String32 *content)
{
    OSTime time = os_time_get_now();

    S2C_ChatMessage *chat_message = mem_arena_push(&s_arena, sizeof(S2C_ChatMessage));

    String32 *username_copy = string32_create_from_string32(&s_arena, username);
    chat_message->username = (String32*)((u8*)username_copy - s_arena.size_used);

    String32 *content_copy = string32_create_from_string32(&s_arena, content);
    chat_message->content = (String32*)((u8*)content_copy - s_arena.size_used);

    chat_message->epoch_time_seconds = time.seconds;
    chat_message->epoch_time_nanoseconds = time.nanoseconds;

    chat_message->header.type = S2C_CHAT_MESSAGE;
    chat_message->header.size = s_arena.size_used;

    os_net_secure_stream_send(s_secure_stream, s_arena.memory, s_arena.size_used);
    mem_arena_reset(&s_arena);
}


void
s2c_user_update(String32 *username, u32 online_status)
{
    S2C_UserUpdate *user_update = mem_arena_push(&s_arena, sizeof(S2C_UserUpdate));

    user_update->status = online_status;

    String32 *username_copy = string32_create_from_string32(&s_arena, username);
    user_update->username = (String32*)((u8*)username_copy - s_arena.size_used);

    user_update->header.type = S2C_USER_UPDATE;
    user_update->header.size = s_arena.size_used;

    os_net_secure_stream_send(s_secure_stream, s_arena.memory, s_arena.size_used);
    mem_arena_reset(&s_arena);
}


void
s2c_login(u32 login_result)
{
    S2C_Login *login = mem_arena_push(&s_arena, sizeof(S2C_Login));
    login->login_result = login_result;

    login->header.type = S2C_LOGIN;
    login->header.size = s_arena.size_used;

    os_net_secure_stream_send(s_secure_stream, s_arena.memory, s_arena.size_used);
    mem_arena_reset(&s_arena);
}


void
messages_init(MemArena *arena, OSNetSecureStream *secure_stream)
{
    s_arena = mem_arena_make_subarena(arena, MESSAGES_MAX_PACKAGE_SIZE);
    s_secure_stream = secure_stream;
}

