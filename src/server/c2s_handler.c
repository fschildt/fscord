#include <basic/basic.h>
#include <messages/messages.h>
#include <server/client_connections.h>
#include <server/c2s_handler.h>
#include <server/s2c_sender.h>


internal_fn void
handle_c2s_chat_message(ClientConnections *conns, ClientConnection *conn)
{
    // Todo: verify package size
    C2S_ChatMessage *chat_message = (C2S_ChatMessage*)conn->recv_buff;
    chat_message->content = (void*)chat_message + (size_t)chat_message->content;

    Time now = os_time_get_now();

    // Todo: make proper groups
    for (size_t i = 0; i < conns->max_connection_count; i++) {
        ClientConnection *connection = conns->connections + i;
        if (connection->username->len > 0) {
            send_s2c_chat_message(connection, conn->username, chat_message->content, now);
        }
    }
}


internal_fn void
handle_c2s_login(ClientConnections *conns, ClientConnection *conn)
{
    // init package
    C2S_Login *login = (C2S_Login*)conn->recv_buff;
    login->username = (void*)login + (size_t)login->username;
    login->password = (void*)login + (size_t)login->password;


    // verify package
    if (login->username->len <= 0) {
        printf("handle_c2s_login error: username len %d is invalid\n", login->username->len);
    }
    if (login->username->len > MESSAGES_MAX_USERNAME_LEN) {
        printf("handle_c2s_login error: username len %d/%d\n", login->username->len, MESSAGES_MAX_USERNAME_LEN);
        return; // Todo: rm connection
    }
    if (login->username->len > MESSAGES_MAX_PASSWORD_LEN) {
        printf("handle_c2s_login error: password len %d/%d\n", login->password->len, MESSAGES_MAX_PASSWORD_LEN);
        return; // Todo: rm connection
    }
    size_t message_size = sizeof(C2S_Login) + 2*sizeof(String32) + sizeof(u32) * (login->username->len + login->password->len);
    if (message_size != conn->recv_buff_size_used) {
        printf("handle_c2s_login error: message size is %zu/%d\n", message_size, conn->recv_buff_size_used);
        return; // Todo: rm connection
    }


    // temporary check if username already connected (use a hashmap for this)
    b32 login_success = true;
    for (size_t i = 0; i < conns->max_connection_count; i++) {
        ClientConnection *connection = conns->connections + i;
        if (string32_equal(connection->username, login->username)) {
            login_success = false;
            break;
        }
    }


    // login
    for (size_t i = 0; i < login->username->len; i++) {
        conn->username->codepoints[i] = login->username->codepoints[i];
    }
    conn->username->len = login->username->len;


    if (!login_success) {
        send_s2c_login(conn, S2C_LOGIN_ERROR);
    }


    send_s2c_login(conn, S2C_LOGIN_SUCCESS);

    // send everyone else's user update to conn
    // Todo: make proper groups
    for (size_t i = 0; i < conns->max_connection_count; i++) {
        ClientConnection *connection = conns->connections + i;
        if (string32_equal(connection->username, conn->username)) {
            continue;
        }
        if (connection->username->len > 0) {
            send_s2c_user_update(conn, connection->username, S2C_USER_UPDATE_ONLINE);
        }
    }

    // send conn's user update to everyone else
    // Todo: make proper groups
    for (size_t i = 0; i < conns->max_connection_count; i++) {
        ClientConnection *connection = conns->connections + i;
        if (connection->username->len > 0) {
            send_s2c_user_update(connection, conn->username, S2C_USER_UPDATE_ONLINE);
        }
    }

    // Todo: make function string32_printf
    printf("<");
    string32_print(conn->username);;
    printf("> connected to the server\n");
}


b32
handle_c2s(ClientConnections *conns, ClientConnection *conn)
{
    // recv header
    if (conn->recv_buff_size_used < sizeof(MessageHeader)) {
        size_t size_to_recv = sizeof(MessageHeader) - conn->recv_buff_size_used;
        i64 size_recvd = os_net_secure_stream_recv(conn->secure_stream_id, conn->recv_buff + conn->recv_buff_size_used, size_to_recv);
        if (size_recvd < 0) {
            return false;
        }
        else if (size_recvd == 0) {
            return false;
        }

        conn->recv_buff_size_used += size_recvd;
        if (conn->recv_buff_size_used < sizeof(MessageHeader)) {
            return true;
        }
    }


    // recv body
    MessageHeader *header = (MessageHeader*)conn->recv_buff;
    if (conn->recv_buff_size_used < header->size) {
        size_t size_to_recv = header->size - conn->recv_buff_size_used;
        i64 size_recvd = os_net_secure_stream_recv(conn->secure_stream_id, conn->recv_buff + conn->recv_buff_size_used, size_to_recv);
        if (size_recvd < 0) {
            return false;
        }
        else if (size_recvd == 0) {
            return false;
        }

        conn->recv_buff_size_used += size_recvd;
        if (conn->recv_buff_size_used < header->size) {
            return true;
        }
    }


    // dispatch
    switch (header->type) {
        case C2S_LOGIN:        handle_c2s_login(conns, conn);        break;
        case C2S_CHAT_MESSAGE: handle_c2s_chat_message(conns, conn); break;
    }


    // cleanup
    conn->recv_buff_size_used = 0;
    return true;
}

