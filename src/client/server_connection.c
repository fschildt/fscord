#include "client/server_connection.h"
#include <basic/basic.h>
#include <client/fscord.h>
#include <crypto/rsa.h>
#include <messages/messages.h>
#include <string.h>


typedef struct {
    char *address;
    u16 port;
    ServerConnectionStatus status; // Todo: use this in code
    EVP_PKEY *client_rsa_public;
    EVP_PKEY *server_rsa_public;
    OSNetSecureStream *secure_stream;
    u32 package_size;
    char package_data[MESSAGES_MAX_PACKAGE_SIZE];
} ServerConnection;


internal_var struct Fscord *s_fscord;
internal_var MemArena *s_trans_arena;
internal_var ServerConnection *s_server_connection;


internal_fn void
handle_s2c_user_update()
{
    S2C_UserUpdate *user_update = (S2C_UserUpdate*)s_server_connection->package_data;
    user_update->username = (String32*)(user_update + 1);
    if (user_update->status == S2C_USER_UPDATE_ONLINE) {
        session_add_user(s_fscord->session, user_update->username);
    } else {
        session_rm_user(s_fscord->session, user_update->username);
    }
}


internal_fn void
handle_s2c_chat_message()
{
    S2C_ChatMessage *chat_message = (S2C_ChatMessage*)s_server_connection->package_data;
    chat_message->username = (String32*)((u8*)chat_message + (size_t)chat_message->username);
    chat_message->content  = (String32*)((u8*)chat_message + (size_t)chat_message->content);
    Time time = {chat_message->epoch_time_seconds, chat_message->epoch_time_nanoseconds};
    session_add_chat_message(s_fscord->session, time, chat_message->username, chat_message->content);
}


internal_fn void
handle_s2c_login()
{
    S2C_Login *login = (S2C_Login*)s_server_connection->package_data;
    if (login->login_result == S2C_LOGIN_SUCCESS) {
        s_server_connection->status = SERVER_CONNECTION_ESTABLISHED;
    }
}


internal_fn b32
handle_s2c()
{
    if (!os_net_secure_stream_recv(s_server_connection->secure_stream, s_server_connection->package_data, sizeof(MessageHeader))) {
        return false;
    }

    MessageHeader *header = (MessageHeader*)s_server_connection->package_data;
    size_t size_to_read = header->size - sizeof(MessageHeader);
    if (!os_net_secure_stream_recv(s_server_connection->secure_stream, s_server_connection->package_data + sizeof(*header), size_to_read)) {
        return false;
    }

    switch (header->type) {
        case S2C_LOGIN:        handle_s2c_login();        break;
        case S2C_CHAT_MESSAGE: handle_s2c_chat_message(); break;
        case S2C_USER_UPDATE:  handle_s2c_user_update();  break;
        InvalidDefaultCase;
    }

    return true;
}


void
server_connection_handle_events()
{
    b32 handled = true;
    while (handled) {
        handled = handle_s2c();
    }
}


void
server_connection_terminate()
{
    os_net_secure_stream_close(s_server_connection->secure_stream);

    if (s_server_connection->client_rsa_public) {
        rsa_destroy(s_server_connection->client_rsa_public);
    }
    InvalidCodePath; // Todo...
}


b32
server_connection_establish(char *address, u16 port, EVP_PKEY *server_rsa_public)
{
    s_server_connection->secure_stream = os_net_secure_stream_connect(address, port, server_rsa_public);
    if (!s_server_connection->secure_stream) {
        return false;
    }

    return true;
}


ServerConnectionStatus
server_connection_get_status()
{
    return s_server_connection->status;
}


void
server_connection_create(MemArena *arena, struct Fscord *fscord)
{
    s_server_connection = mem_arena_push(arena, sizeof(*s_server_connection));

    // initialize
    memset(s_server_connection, 0, sizeof(*s_server_connection));

    s_fscord = fscord;
    s_trans_arena = &fscord->trans_arena;
}

