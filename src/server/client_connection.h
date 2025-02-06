#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <basic/basic.h>
#include <basic/string32.h>
#include <os/os.h>
#include <messages/messages.h>

typedef struct {
    u32 secure_stream_id;
    u32 recv_buffer_size;
    u32 recv_buffer[1408];
    String32Buffer username;
} ClientConnection;

#define CLIENT_CONNECTION_INVALID_ID U32_MAX

b32  client_connections_create(MemArena *arena);
b32  client_connection_add(u32 secure_stream_id); // remove handled internally

void send_s2c_login(u32 login_result);
void send_s2c_user_update(String32 *username, u32 online_status);
void send_s2c_chat_message(String32 *username, String32 *content);


#endif // CLIENT_CONNECTION_H
