#ifndef CLIENT_CONNECTIONS_H
#define CLIENT_CONNECTIONS_H

#include <basic/basic.h>
#include <basic/string32.h>
#include <os/os.h>
#include <messages/messages.h>


#define CLIENT_CONNECTION_INVALID_ID U32_MAX


typedef struct {
    u32 id;
    u32 secure_stream_id;
    u32 recv_buff_size_used;
    u8 recv_buff[1408];

    String32 *username;
    u8 username_buff[sizeof(String32) + MESSAGES_MAX_USERNAME_LEN * sizeof(u32)];
} ClientConnection;


typedef struct {
    EVP_PKEY *server_rsa_pri;
    u32 listener_id;
    int epoll_fd;

    size_t max_connection_count;
    ClientConnection *connections;

    u32 free_id_count;
    u32 *free_ids;
} ClientConnections;



ClientConnections *client_connections_create(MemArena *arena, u16 port);
void client_connections_manage(ClientConnections *connections);

ClientConnection* client_connection_id_to_ptr(ClientConnections *connections, u32 id);


#endif // CLIENT_CONNECTIONS_H
