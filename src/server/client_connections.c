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
client_connection_id_to_ptr(ClientConnections *conns, u32 id)
{
    ClientConnection *conn = &conns->connections[id];
    return conn;
}


void
client_connection_rm(ClientConnections *conns, u32 connection_id)
{
    ClientConnection *conn = client_connection_id_to_ptr(conns, connection_id);

    int fd = os_net_secure_stream_get_fd(conn->secure_stream_id);
    int deleted = epoll_ctl(conns->epoll_fd, EPOLL_CTL_DEL, fd, 0); 
    if (deleted == -1) {
        perror("epoll_ctl<del>:");
    }

    conns->free_ids[conns->free_id_count++] = connection_id;
}


u32
client_connection_add(ClientConnections *conns, u32 secure_stream_id)
{
    if (conns->free_id_count == 0) {
        return CLIENT_CONNECTION_INVALID_ID;
    }
    u32 conn_id = conns->free_ids[--conns->free_id_count];
    ClientConnection *conn = client_connection_id_to_ptr(conns, conn_id);
    conn->id = conn_id;


    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.u32 = conn_id;

    int fd = os_net_secure_stream_get_fd(secure_stream_id);

    if (epoll_ctl(conns->epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
        conns->free_ids[conns->free_id_count++] = conn_id;
        perror("epoll_ctl<add>:");
        return false;
    }


    conn->secure_stream_id = secure_stream_id;
    conn->recv_buff_size_used = 0;

    return conn_id;
}


internal_fn void
handle_client_event(ClientConnections *conns, struct epoll_event event)
{
    u32 conn_id = event.data.u32;

    ClientConnection *conn = client_connection_id_to_ptr(conns, conn_id);
    int fd = os_net_secure_stream_get_fd(conn->secure_stream_id);

    // error states
    if (event.events & EPOLLERR) {
        client_connection_rm(conns, conn_id);
        printf("EPOLLERR occured for client_id = %d, fd = %d\n", conn_id, fd);
    }
    else if (event.events & EPOLLHUP) {
        client_connection_rm(conns, conn_id);
        printf("EPOLLHUP occured for client_id = %d, fd = %d\n", conn_id, fd);
    }
    // read state
    if (event.events & EPOLLIN) {
        if (!handle_c2s(conns, conn)) {
            return client_connection_rm(conns, conn_id);
        }
    }
}


internal_fn void
handle_listener_event(ClientConnections *conns, struct epoll_event event)
{
    if (event.events & (EPOLLHUP | EPOLLERR)) {
        printf("listener failed\n");
        exit(0);
    }
    if (event.events & EPOLLIN) {
        u32 secure_stream_id = os_net_secure_stream_accept(conns->listener_id);
        if (secure_stream_id == OS_NET_SECURE_STREAM_ID_INVALID) {
            return;
        }

        client_connection_add(conns, secure_stream_id);
    }
    else {
        printf("unhandled listener event %d\n", event.events);
    }
}


void
client_connections_manage(ClientConnections *conns)
{
    struct epoll_event listener_events;
    listener_events.data.u32 = conns->listener_id;
    listener_events.events = EPOLLIN;
    int listener_fd = os_net_secure_stream_get_fd(conns->listener_id);
    if (epoll_ctl(conns->epoll_fd, EPOLL_CTL_ADD, listener_fd, &listener_events) < 0) {
        perror("epoll_ctl add for listener fd:");
        return;
    }

    for (;;) {
        struct epoll_event events[conns->max_connection_count];
        i32 event_count = epoll_wait(conns->epoll_fd, events, ARRAY_COUNT(events), -1);
        if (event_count < -1) {
            perror("epoll_wait:");
            continue;
        }

        if (event_count > 0) {
            printf("event.events = %d\n", events[0].events);
        }

        for (size_t i = 0; i < event_count; i++) {
            // Todo: use a threadpool to deal with these events
            if (events[i].data.u32 == conns->listener_id) {
                handle_listener_event(conns, events[i]);
            } else {
                handle_client_event(conns, events[i]);
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


    ClientConnections *conns = mem_arena_push(arena, sizeof(ClientConnections));

    conns->server_rsa_pri = rsa_create_via_file(arena, "./server_rsa_pri.pem", false);

    conns->listener_id = os_net_secure_stream_listen(port, conns->server_rsa_pri);
    if (conns->listener_id == OS_NET_SECURE_STREAM_ID_INVALID) {
        return 0;
    }


    size_t push_size = sizeof(ClientConnection) * max_user_count;
    conns->max_connection_count = max_user_count;
    conns->connections = mem_arena_push(arena, push_size);

    ClientConnection *conn = conns->connections;
    for (size_t i = 0; i < conns->max_connection_count; i++) {
        conn->secure_stream_id = CLIENT_CONNECTION_INVALID_ID;
        conn->recv_buff_size_used = 0;

        conn->username = (String32*)conn->username_buff;
        conn->username->len = 0;

        conn++;
    }


    conns->used_id_count = 0;
    conns->used_ids = mem_arena_push(arena, max_user_count * sizeof(*conns->used_ids));

    conns->free_id_count = max_user_count;
    conns->free_ids = mem_arena_push(arena, max_user_count * sizeof(*conns->free_ids));
    for (size_t i = 0; i < max_user_count; i++) {
        conns->free_ids[i] = i;
    }


    conns->epoll_fd = epoll_create(max_user_count);
    if (conns->epoll_fd < 0) {
        perror("epoll_create:");
        return 0;
    }


    return conns;
}

