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

    // send message to everyone
    for (u32 id_idx = 0; id_idx < conns->used_id_count; id_idx++) {
        u32 id = conns->used_ids[id_idx];
        ClientConnection *conn_it = client_connection_id_to_ptr(conns, id);

        send_s2c_chat_message(conn_it, conn->username, chat_message->content);
    }
}


internal_fn void
handle_c2s_login(ClientConnections *conns, ClientConnection *conn)
{
    // Todo: verify package size
    C2S_Login *login = (C2S_Login*)conn->recv_buff;
    login->username = (void*)login + (size_t)login->username;
    login->password = (void*)login + (size_t)login->password;

    // Todo: use a hashmap
    u32 login_success = true;
    for (u32 conn_id_index = 0; conn_id_index < conns->used_id_count; conn_id_index++) {
        u32 id = conns->used_ids[conn_id_index];
        ClientConnection *conn_it = client_connection_id_to_ptr(conns, id);
        if (string32_equal(conn_it->username, conn->username)) {
            login_success = false;
            break;
        }
    }

    if (login_success) {
        send_s2c_login(conn, S2C_LOGIN_SUCCESS);
    } else {
        send_s2c_login(conn, S2C_LOGIN_ERROR);
    }
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

