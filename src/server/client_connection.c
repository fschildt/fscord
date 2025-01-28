#include "basic/basic.h"
#include <server/client_connection.h>
#include <messages/messages.h>
#include <os/os.h>

// Todo: use <os/os.h> functions? xd
#include <sys/epoll.h>
#include <pthread.h>
#include <unistd.h>


internal_var ClientConnection **s_connection_ptrs;
internal_var int s_epoll_fd;

internal_var pthread_mutex_t s_mutex_alloc; // Todo: figure out if the lock needs to envelop epoll_ctl additionally to the free id alloc/free
internal_var volatile i16 s_free_id_count;
internal_var volatile i16 *s_free_ids;


internal_fn void
handle_c2s(ClientConnection *connection)
{
    u32 package_size;
    char package_data[MESSAGES_MAX_PACKAGE_SIZE];
    // Todo: ...
}


void *
client_connections_runner(void *data)
{
    struct epoll_event events[MESSAGES_MAX_USER_COUNT];
    for (;;) {
        i32 event_count = epoll_wait(s_epoll_fd, events, MESSAGES_MAX_USER_COUNT, -1);
        if (event_count == -1) {
            perror("epoll_wait:");
        }

        for (size_t i = 0; i < event_count; i++) {
            i16 connection_id = events[i].data.u32;
            ClientConnection *connection = s_connection_ptrs[connection_id];

            if (events[i].events & EPOLLIN) {
                handle_c2s(connection); // Todo: maybe even create another thread for this?
            } else {
                printf("unhandled epoll event %d\n", events[i].events);
            }
        }
    }
}


void
client_connection_deinit_and_free(i16 id)
{
    pthread_mutex_lock(&s_mutex_alloc);

    assert(id >= 0 && id < MESSAGES_MAX_USER_COUNT);
    s_free_ids[s_free_id_count++] = id;


    ClientConnection *connection = s_connection_ptrs[id];
    int fd = os_net_secure_stream_get_fd(connection->secure_stream);

    int deleted = epoll_ctl(s_epoll_fd, EPOLL_CTL_DEL, fd, 0); 
    if (deleted == -1) {
        perror("epoll_ctl<del>:");
    }

    pthread_mutex_unlock(&s_mutex_alloc);
}


i16
client_connection_alloc_and_init(OSNetSecureStream *secure_stream)
{
    pthread_mutex_lock(&s_mutex_alloc);

    if (s_free_id_count <= 0) {
        return -1;
    }

    i16 id = s_free_ids[--s_free_id_count];


    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.u32 = (u32)id;

    int fd = os_net_secure_stream_get_fd(secure_stream);

    if (epoll_ctl(s_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
        client_connection_deinit_and_free(id);
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
    assert(MESSAGES_MAX_USER_COUNT >= I16_MAX);

    s_connection_ptrs = mem_arena_push(arena, max_user_count * sizeof(*s_connection_ptrs));
    for (size_t i = 0; i < max_user_count; i++) {
        s_connection_ptrs[i] = mem_arena_push(arena, sizeof(ClientConnection));
        s_connection_ptrs[i]->username = string32_buffer_create(arena, MESSAGES_MAX_USERNAME_LEN);
        s_connection_ptrs[i]->secure_stream = 0;
    }


    s_free_ids = mem_arena_push(arena, max_user_count * sizeof(*s_free_ids));
    for (size_t i = 0; i < MESSAGES_MAX_USER_COUNT; i++) {
        s_free_ids[i] = i;
    }
    s_free_id_count = MESSAGES_MAX_USER_COUNT;


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

