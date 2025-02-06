#include <basic/basic.h>
#include <os/os.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


typedef struct {
    int fd;
} OSNetStream;


internal_var u32 s_max_stream_count;
internal_var OSNetStream *s_streams;

internal_var u32 s_free_id_count;
internal_var u32 *s_free_ids;



internal_fn void
os_net_stream_free(u32 stream_id)
{
    s_free_ids[s_free_id_count] = stream_id;
    s_free_id_count += 1;
}

internal_fn u32
os_net_stream_alloc()
{
    if (s_free_id_count == 0) {
        return OS_NET_STREAM_ID_INVALID;
    }

    u32 id = s_free_ids[s_free_id_count-1];
    s_free_id_count -= 1;

    return id;
}



b32
os_net_stream_send(u32 stream_id, void *buffer, size_t size)
{
    OSNetStream *stream = &s_streams[stream_id];

    size_t sent = send(stream->fd, buffer, size, 0);
    if (sent == -1) {
        printf("send failed\n");
        return false;
    } else if (sent < size) {
        printf("send only sent %ld/%ld bytes\n", sent, size);
        return false;
    }
    return true;
}

b32
os_net_stream_recv(u32 stream_id, void *buffer, size_t size)
{
    OSNetStream *stream = &s_streams[stream_id];

    ssize_t recvd = recv(stream->fd, buffer, size, 0);
    if (unlikely(recvd == -1 || recvd != size)) {
        return false;
    }

    return true;
}

int
os_net_stream_get_fd(u32 stream_id)
{
    OSNetStream *stream = &s_streams[stream_id];
    return stream->fd;
}

void
os_net_stream_close(u32 stream_id)
{
    OSNetStream *stream = &s_streams[stream_id];
    close(stream->fd);
}

u32
os_net_stream_accept(u32 listener_id)
{
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);

    OSNetStream *listener = &s_streams[listener_id];
    fd = accept(listener->fd, (struct sockaddr*)&addr, &addr_size);
    if (fd == -1) {
        printf("accept failed\n");
        return 0;
    }

    u32 stream_id = os_net_stream_alloc();
    OSNetStream *stream = &s_streams[stream_id];
    stream->fd = fd;
    return stream_id;
}

u32
os_net_stream_listen(u16 port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket()");
        return 0;
    }

    int enable_reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable_reuse, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        close(fd);
        return 0;
    }

    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind()");
        close(fd);
        return 0;
    }

    int backlog = 128;
    if (listen(fd, backlog) == -1) {
        perror("listen()");
        close(fd);
        return 0;
    }

    u32 listener_id = os_net_stream_alloc();
    OSNetStream *stream = &s_streams[listener_id];
    stream->fd = fd;

    return listener_id;
}

u32
os_net_stream_connect(char *address, u16 port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        printf("cant open socket\n");
        return 0;
    }

    // note: connect binds a local address automatically
    struct sockaddr_in target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(1338);
    target_addr.sin_addr.s_addr = inet_addr(address);
    if (connect(fd, (struct sockaddr*)&target_addr, sizeof(target_addr)) == -1) {
        printf("connect failed\n");
        close(fd);
        return 0;
    }

    u32 stream_id = os_net_stream_alloc();
    OSNetStream *stream = &s_streams[stream_id];
    stream->fd = fd;

    return stream_id;
}

void
os_net_streams_init(MemArena *arena, size_t max_stream_count)
{
    s_max_stream_count = max_stream_count;
    s_streams = mem_arena_push(arena, max_stream_count * sizeof(OSNetStream));

    s_free_id_count = max_stream_count;
    s_free_ids = mem_arena_push(arena, max_stream_count * sizeof(u32));
    for (size_t i = 0; i < max_stream_count; i++) {
        s_free_ids[i] = i;
    }
}

