#include "basic/basic.h"
#include <os/os.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


struct OSNetStream {
    int fd;
};

typedef struct {
    size_t free_stream_count;
    size_t max_stream_count;
    OSNetStream *streams; // other streams follow in memory
    OSNetStream **free_streams;
} OSNetStreamsData;


internal_var OSNetStreamsData s_data;


size_t
os_net_stream_get_size()
{
    return sizeof(OSNetStream);
}


internal_fn void
os_net_stream_free(OSNetStream *stream)
{
    s_data.free_streams[s_data.free_stream_count++] = stream;
}

internal_fn OSNetStream *
os_net_stream_alloc()
{
    if (unlikely(s_data.free_stream_count == 0)) {
        return 0;
    }

    OSNetStream *stream = s_data.free_streams[--s_data.free_stream_count];
    return stream;
}

void
os_net_streams_init(MemArena *arena, size_t max_stream_count)
{
    s_data.free_stream_count = max_stream_count;
    s_data.max_stream_count = max_stream_count;
    s_data.streams = mem_arena_push(arena, max_stream_count * sizeof(OSNetStream));
    s_data.free_streams = mem_arena_push(arena, max_stream_count * sizeof(OSNetStream*));
    for (size_t i = 0; i < max_stream_count; i++) {
        s_data.streams[i].fd = -1; // probably redundant/unnecessary
    }
    for (size_t i = 0; i < max_stream_count; i++) {
        s_data.free_streams[i] = &s_data.streams[i];
    }
}



b32
os_net_stream_send(OSNetStream *stream, void *buffer, size_t size)
{
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
os_net_stream_recv(OSNetStream *stream, void *buffer, size_t size)
{
    ssize_t recvd = recv(stream->fd, buffer, size, 0);
    if (unlikely(recvd == -1 || recvd != size)) {
        return false;
    }

    return true;
}

void
os_net_stream_close(OSNetStream *stream)
{
    close(stream->fd);
}

OSNetStream *
os_net_stream_accept(OSNetStream *listener)
{
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);

    fd = accept(listener->fd, (struct sockaddr*)&addr, &addr_size);
    if (fd == -1) {
        printf("accept failed\n");
        return 0;
    }

    OSNetStream *stream = os_net_stream_alloc();
    stream->fd = fd;
    return stream;
}

OSNetStream *
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

    OSNetStream *stream = os_net_stream_alloc();
    stream->fd = fd;
    return stream;
}

OSNetStream *
os_net_stream_connect(char *address, u16 port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (!fd) {
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

    OSNetStream *stream = os_net_stream_alloc();
    stream->fd = fd;
    return stream;
}

OSNetStream *
os_net_stream_create(MemArena *arena)
{
    OSNetStream *stream = mem_arena_push(arena, sizeof(OSNetStream));
    return stream;
}

