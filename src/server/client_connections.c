#include <basic/basic.h>
#include <messages/messages.h>
#include <crypto/rsa.h>
#include <os/os.h>
#include <server/client_connections.h>
#include <server/c2s_handler.h>

// Todo: use <os/os.h> functions? xd
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>



ClientConnection *
client_connection_id_to_ptr(ClientConnections *connections, u32 id)
{
    ClientConnection *connection = &connections->connections[id];
    return connection;
}


void
client_connection_rm(ClientConnections *connections, u32 connection_id)
{
    ClientConnection *connection = client_connection_id_to_ptr(connections, connection_id);

    int fd = os_net_secure_stream_get_fd(connection->secure_stream_id);
    int deleted = epoll_ctl(connections->epoll_fd, EPOLL_CTL_DEL, fd, 0); 
    if (deleted == -1) {
        perror("epoll_ctl<del>:");
    }

    connections->free_ids[connections->free_id_count++] = connection_id;
}


u32
client_connection_add(ClientConnections *connections, u32 secure_stream_id)
{
    if (connections->free_id_count == 0) {
        return CLIENT_CONNECTION_INVALID_ID;
    }
    u32 connection_id = connections->free_ids[--connections->free_id_count];


    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.u32 = (u32)connection_id;

    int fd = os_net_secure_stream_get_fd(secure_stream_id);

    if (epoll_ctl(connections->epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
        connections->free_ids[connections->free_id_count++] = connection_id;
        perror("epoll_ctl<add>:");
        return false;
    }


    return connection_id;
}


internal_fn void
handle_client_event(ClientConnections *connections, struct epoll_event event)
{
    // Todo: handle all events
    if (event.events & EPOLLIN) {
        ClientConnection *connection = client_connection_id_to_ptr(connections, event.data.u32);
        handle_c2s(connections, connection);
    } else {
        printf("unhandled client event %d\n", event.events);
    }
}


internal_fn void
handle_listener_event(ClientConnections *connections, struct epoll_event event)
{
    // Todo: handle all events
    if (event.events & EPOLLIN) {
        u32 secure_stream_id = os_net_stream_accept(connections->listener_fd);
        if (secure_stream_id == OS_NET_STREAM_ID_INVALID) {
            return;
        }

        client_connection_add(connections, secure_stream_id);
    }
    else {
        printf("unhandled listener event %d\n", event.events);
    }
}


void
client_connections_manage(ClientConnections *connections)
{
    struct epoll_event listener_events;
    listener_events.data.fd = connections->listener_fd;
    listener_events.events = EPOLLIN;
    if (epoll_ctl(connections->epoll_fd, EPOLL_CTL_ADD, connections->listener_fd, &listener_events) < 0) {
        perror("epoll_ctl add for listener fd:");
        return;
    }

    for (;;) {
        struct epoll_event events[connections->max_connection_count];
        i32 event_count = epoll_wait(connections->epoll_fd, events, ARRAY_COUNT(events), -1);
        if (event_count < -1) {
            perror("epoll_wait:");
            continue;
        }

        for (size_t i = 0; i < event_count; i++) {
            // Todo: use a threadpool to deal with these events
            if (events[i].data.u32 == connections->listener_fd) {
                handle_listener_event(connections, events[i]);
            } else {
                handle_client_event(connections, events[i]);
            }
        }
    }
}


ClientConnections *
client_connections_create(MemArena *arena, u16 port)
{
    u32 max_user_count = MESSAGES_MAX_USER_COUNT;
    size_t max_username_len = MESSAGES_MAX_USERNAME_LEN;


    os_net_secure_streams_init(arena, max_user_count + 1);


    ClientConnections *connections = mem_arena_push(arena, sizeof(ClientConnections));

    connections->server_rsa_pri = rsa_create_via_file(arena, "./server_rsa_pri.pem", false);

    u32 listener_fd = os_net_secure_stream_listen(port, connections->server_rsa_pri);
    if (listener_fd == OS_NET_SECURE_STREAM_ID_INVALID) {
        return 0;
    }


    size_t push_size = sizeof(ClientConnection) * max_user_count;
    connections->max_connection_count = max_user_count;
    connections->connections = mem_arena_push(arena, push_size);

    ClientConnection *conn = connections->connections;
    for (size_t i = 0; i < connections->max_connection_count; i++) {
        conn->secure_stream_id = CLIENT_CONNECTION_INVALID_ID;
        conn->recv_buffer_size = 0;

        conn->username = (String32*)conn->username_buff;
        conn->username->len = 0;

        conn++;
    }


    connections->used_id_count = 0;
    connections->used_ids = mem_arena_push(arena, max_user_count * sizeof(*connections->used_ids));

    connections->free_id_count = max_user_count;
    connections->free_ids = mem_arena_push(arena, max_user_count * sizeof(*connections->free_ids));
    for (size_t i = 0; i < max_user_count; i++) {
        connections->free_ids[i] = i;
    }


    connections->epoll_fd = epoll_create(max_user_count);
    if (connections->epoll_fd < 0) {
        perror("epoll_create:");
        return 0;
    }


    return connections;
}

