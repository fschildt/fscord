#ifndef CLIENT_CONNECTIONS_H
#define CLIENT_CONNECTIONS_H

#include <basic/basic.h>
#include <basic/string32.h>
#include <os/os.h>
#include <messages/messages.h>

typedef struct {
    u32 secure_stream_id;
    u32 recv_buffer_size;
    u32 recv_buffer[1408];

    // Todo: find some cleaner solution to this :(
    String32 *username;
    u8 username_buff[sizeof(String32) + MESSAGES_MAX_USERNAME_LEN * sizeof(u32)];
} ClientConnection;

typedef struct {
    EVP_PKEY *server_rsa_pri;
    int listener_fd;
    int epoll_fd;

    size_t max_connection_count;
    ClientConnection *connections;

    // Note: used_ids are there to loop over tight groups
    // Note: free_ids are there to allocate/free
    u32 used_id_count;
    u32 free_id_count;

    u32 *used_ids;
    u32 *free_ids;
} ClientConnections;

#define CLIENT_CONNECTION_INVALID_ID U32_MAX

ClientConnections *client_connections_create(MemArena *arena, u16 port);

void client_connections_manage(ClientConnections *connections);
ClientConnection* client_connection_id_to_ptr(ClientConnections *connections, u32 id);


#endif // CLIENT_CONNECTIONS_H
