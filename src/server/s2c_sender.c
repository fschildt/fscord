#include <server/s2c_sender.h>

internal_var MemArena s_send_arena;

void
send_s2c_chat_message(ClientConnection *conn, String32 *username, String32 *content)
{
    OSTime time = os_time_get_now();
    MemArena *arena = &s_send_arena;


    S2C_ChatMessage *chat_message = mem_arena_push(arena, sizeof(S2C_ChatMessage));

    String32 *username_copy = string32_create_from_string32(arena, username);
    chat_message->username = (String32*)((u8*)username_copy - arena->size_used);

    String32 *content_copy = string32_create_from_string32(arena, content);
    chat_message->content = (String32*)((u8*)content_copy - arena->size_used);

    chat_message->epoch_time_seconds = time.seconds;
    chat_message->epoch_time_nanoseconds = time.nanoseconds;

    chat_message->header.type = S2C_CHAT_MESSAGE;
    chat_message->header.size = arena->size_used;


    os_net_secure_stream_send(conn->secure_stream_id, arena->memory, arena->size_used);
    mem_arena_reset(arena);
}


void
send_s2c_user_update(ClientConnection *conn, String32 *username, u32 online_status)
{
    MemArena *arena = &s_send_arena;


    S2C_UserUpdate *user_update = mem_arena_push(arena, sizeof(S2C_UserUpdate));

    user_update->status = online_status;

    String32 *username_copy = string32_create_from_string32(arena, username);
    user_update->username = (String32*)((u8*)username_copy - arena->size_used);

    user_update->header.type = S2C_USER_UPDATE;
    user_update->header.size = arena->size_used;


    os_net_secure_stream_send(conn->secure_stream_id, arena->memory, arena->size_used);
    mem_arena_reset(arena);
}


void
send_s2c_login(ClientConnection *conn, u32 login_result)
{
    MemArena *arena = &s_send_arena;


    S2C_Login *login = mem_arena_push(arena, sizeof(S2C_Login));
    login->login_result = login_result;

    login->header.type = S2C_LOGIN;
    login->header.size = arena->size_used;


    os_net_secure_stream_send(conn->secure_stream_id, arena->memory, arena->size_used);
    mem_arena_reset(arena);
}


void
s2c_sender_init(MemArena *arena)
{
    s_send_arena = mem_arena_make_subarena(arena, 1408);
}
