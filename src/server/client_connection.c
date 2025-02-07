#include <basic/basic.h>
#include <server/client_connection.h>
#include <messages/messages.h>
#include <os/os.h>

// Todo: use <os/os.h> functions? xd
#include <sys/epoll.h>
#include <pthread.h>
#include <unistd.h>


internal_var size_t s_connection_size;
internal_var u32    s_max_connection_count;
internal_var void   *s_connections;

internal_var volatile u32 s_free_id_count;
internal_var volatile u32 *s_free_ids;
internal_var int s_epoll_fd;
internal_var pthread_mutex_t s_mutex_alloc;

internal_var MemArena s_send_arena;



ClientConnection *
client_connection_id_to_ptr(u32 id)
{
    ClientConnection *result = s_connections + (id * s_connection_size);
    return result;
}



void
s2c_chat_message(ClientConnection *conn, String32 *username, String32 *content)
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
s2c_user_update(ClientConnection *conn, String32 *username, u32 online_status)
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
s2c_login(ClientConnection *conn, u32 login_result)
{
    MemArena *arena = &s_send_arena;


    S2C_Login *login = mem_arena_push(arena, sizeof(S2C_Login));
    login->login_result = login_result;

    login->header.type = S2C_LOGIN;
    login->header.size = arena->size_used;


    os_net_secure_stream_send(conn->secure_stream_id, arena->memory, arena->size_used);
    mem_arena_reset(arena);
}


internal_fn void
handle_c2s_chat_message(ClientConnection *connection)
{
    // Todo: verify package size

    C2S_ChatMessage *chat_message = (C2S_ChatMessage*)connection->recv_buffer;
    chat_message->content = (void*)chat_message + (size_t)chat_message->content;


    // Todo: send answer
    InvalidCodePath;
}


internal_fn void
handle_c2s_login(ClientConnection *connection)
{
    // Todo: verify package size

    C2S_Login *login = (C2S_Login*)connection->recv_buffer;
    login->username = (void*)login + (size_t)login->username;
    login->password = (void*)login + (size_t)login->password;


    // Todo: send answer
    InvalidCodePath;
}


internal_fn void
handle_c2s(ClientConnection *conn)
{
    // Todo: handle partial recv by changing os_net_secure_stream_recv api to return i32


    // recv header
    if (conn->recv_buffer_size < sizeof(MessageHeader)) {
        size_t recv_size = sizeof(MessageHeader) - conn->recv_buffer_size;
        b32 recvd = os_net_secure_stream_recv(conn->secure_stream_id, conn->recv_buffer + conn->recv_buffer_size, recv_size);
        if (!recvd) {
            return;
        }
        conn->recv_buffer_size += recv_size;
    }


    // recv body
    MessageHeader *header = (MessageHeader*)conn->recv_buffer;
    if (conn->recv_buffer_size < header->size) {
        size_t recv_size = header->size - conn->recv_buffer_size;
        b32 recvd = os_net_secure_stream_recv(conn->secure_stream_id, conn->recv_buffer + conn->recv_buffer_size, recv_size);
        if (!recvd) {
            return;
        }
        conn->recv_buffer_size += recv_size;
    }


    // dispatch
    switch (header->type) {
        case C2S_LOGIN:        handle_c2s_login(conn);        break;
        case C2S_CHAT_MESSAGE: handle_c2s_chat_message(conn); break;
    }
}


void *
client_connections_runner(void *data)
{
    struct epoll_event events[MESSAGES_MAX_USER_COUNT];
    for (;;) {
        i32 event_count = epoll_wait(s_epoll_fd, events, MESSAGES_MAX_USER_COUNT, -1);
        if (event_count > 0) {
            printf("epoll event_count = %d\n", event_count);
        }
        if (event_count == -1) {
            perror("epoll_wait:");
        }

        for (size_t i = 0; i < event_count; i++) {
            u32 conn_id = events[i].data.u32;
            ClientConnection *connection = client_connection_id_to_ptr(conn_id);

            if (events[i].events & EPOLLIN) {
                handle_c2s(connection); // Todo: maybe even create another thread for this?
            } else {
                printf("unhandled epoll event %d\n", events[i].events);
            }
        }
    }
}


internal_fn void
client_connection_free(u32 id)
{
    assert(s_free_id_count < s_max_connection_count);

    s_free_ids[s_free_id_count++] = id;
}


internal_fn u32
client_connection_alloc()
{
    if (s_free_id_count == 0) {
        return CLIENT_CONNECTION_INVALID_ID;
    }

    u32 id = s_free_ids[--s_free_id_count];
    return id;
}


void
client_connection_rm(u32 id)
{
    pthread_mutex_lock(&s_mutex_alloc);


    ClientConnection *connection = client_connection_id_to_ptr(id);

    int fd = os_net_secure_stream_get_fd(connection->secure_stream_id);
    int deleted = epoll_ctl(s_epoll_fd, EPOLL_CTL_DEL, fd, 0); 
    if (deleted == -1) {
        perror("epoll_ctl<del>:");
    }

    client_connection_free(id);


    pthread_mutex_unlock(&s_mutex_alloc);
}


u32
client_connection_add(u32 secure_stream_id)
{
    pthread_mutex_lock(&s_mutex_alloc);


    u32 id = client_connection_alloc();

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.u32 = (u32)id;

    int fd = os_net_secure_stream_get_fd(secure_stream_id);

    if (epoll_ctl(s_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
        client_connection_free(id);
        perror("epoll_ctl<add>:");
        return false;
    }


    pthread_mutex_unlock(&s_mutex_alloc);

    return id;
}


// Todo: return b32 does make no sense since we'll quit anyway. All this error handling makes no sense.
b32
client_connections_create(MemArena *arena)
{
    size_t arena_save = mem_arena_save(arena);
    size_t max_user_count = MESSAGES_MAX_USER_COUNT;
    size_t max_username_len = MESSAGES_MAX_USERNAME_LEN;
    size_t connection_size = sizeof(ClientConnection) + max_username_len * sizeof(u32);


    s_connection_size = connection_size;
    s_max_connection_count = max_user_count;
    s_connections = mem_arena_push(arena, s_max_connection_count * connection_size);

    ClientConnection *conn = s_connections;
    for (size_t i = 0; i < s_max_connection_count; i++) {
        conn->secure_stream_id = CLIENT_CONNECTION_INVALID_ID;
        conn->recv_buffer_size = 0;
        string32_buffer_init_in_place(&conn->username, max_username_len);

        conn = (void*)conn + connection_size;
    }


    s_free_id_count = max_user_count;
    s_free_ids = mem_arena_push(arena, max_user_count * sizeof(*s_free_ids));
    for (size_t i = 0; i < max_user_count; i++) {
        s_free_ids[i] = i;
    }


    s_epoll_fd = epoll_create(max_user_count);
    if (s_epoll_fd < 0) {
        mem_arena_restore(arena, arena_save);
        perror("epoll_create:");
        return false;
    }


    pthread_mutex_init(&s_mutex_alloc, 0);


    pthread_t tid;
    int err = pthread_create(&tid, 0, client_connections_runner, 0);
    if (err != 0) {
        close(s_epoll_fd);
        pthread_mutex_destroy(&s_mutex_alloc);
        mem_arena_restore(arena, arena_save);
        printf("pthread_create error %d\n", err);
        return false;
    }

    return true;
}

