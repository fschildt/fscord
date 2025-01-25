#include "basic/mem_arena.h"
#include <os/os.h>
#include <basic/string32.h>
#include <messages//messages.h>

#include <string.h>

internal_var OSNetSecureStream *s_secure_stream;
internal_var MemArena s_arena;

void
msg_login_request(String32 *password, String32 *username)
{
    MsgLoginRequest *login_request = mem_arena_push(&s_arena, sizeof(MsgLoginRequest));
    login_request->message_type = MSG_TYPE_LOGIN_REQUEST;
    login_request->password_len = password->len;
    login_request->username_len = username->len;

    u32 *password_data = mem_arena_push(&s_arena, password->len * sizeof(u32));
    memcpy(password_data, password->p, password->len);

    u32 *username_data = mem_arena_push(&s_arena, username->len * sizeof(u32));
    memcpy(username_data, username->p, username->len);


    os_net_secure_stream_send(s_secure_stream, s_arena.memory, s_arena.size_used);

    mem_arena_reset(&s_arena);
}

void
msg_online_status(String32 *username, u32 value)
{
    MsgOnlineStatus *online_status = mem_arena_push(&s_arena, sizeof(MsgOnlineStatus));
    online_status->message_type = MSG_TYPE_ONLINE_STATUS;
    online_status->status = value;
    online_status->username_len = username->len;

    u32 *username_data = mem_arena_push(&s_arena, username->len * sizeof(u32));
    memcpy(username_data, username->p, username->len);


    os_net_secure_stream_send(s_secure_stream, s_arena.memory, s_arena.size_used);

    mem_arena_reset(&s_arena);
}

void
msg_login_response(u32 login_result)
{
    MsgLoginResponse *response = mem_arena_push(&s_arena, sizeof(MsgLoginResponse));
    response->message_type = MSG_TYPE_LOGIN_RESPONSE;
    response->login_result = login_result;


    os_net_secure_stream_send(s_secure_stream, s_arena.memory, s_arena.size_used);

    mem_arena_reset(&s_arena);
}

void
msg_chat_message(String32 *content)
{
    OSTime time = os_time_get_now();

    MsgChatMessage *chat_message = mem_arena_push(&s_arena, sizeof(MsgChatMessage));
    chat_message->message_type = MSG_TYPE_CHAT_MESSAGE;
    chat_message->sender_username_len = 0;
    chat_message->content_len = content->len;
    chat_message->epoch_time_seconds = time.seconds;
    chat_message->epoch_time_nanoseconds = time.nanoseconds;

    u32 *content_data = mem_arena_push(&s_arena, content->len * sizeof(u32));
    memcpy(content_data, content->p, content->len);


    os_net_secure_stream_send(s_secure_stream, s_arena.memory, s_arena.size_used);

    mem_arena_reset(&s_arena);
}

void
msg_init(MemArena *arena, OSNetSecureStream *secure_stream)
{
    s_arena = mem_arena_make_subarena(arena, MESSAGES_MAX_PACKAGE_SIZE);
    s_secure_stream = secure_stream;
}

