typedef struct
{
    Sys_Tcp sys_tcp;
    int fd;
} Lin_Tcp;

internal bool
lin_send(Lin_Tcp *tcp, void *buffer, s64 size)
{
    ssize_t sent = send(tcp->fd, buffer, size, MSG_NOSIGNAL);
    if (sent == -1)
    {
        printf("send %ld bytes failed\n", size);
        return false;
    }
    if (sent != size)
    {
        printf("sent only %zd/%ld bytes\n", sent, size);
        return false;
    }
    printf("sent %ld bytes\n", size);
    return true;
}

internal s64
lin_recv(Lin_Tcp *tcp, void *buffer, s64 size)
{
    ssize_t recvd = recv(tcp->fd, buffer, size, 0);
    if (recvd == -1)
    {
        if (errno == EAGAIN)
        {
            return 0;
        }
        else
        {
            printf("recv %ld bytes failed\n", size);
            return -1;
        }
    }
    s64 result = recvd;
    return result;
}

internal void
lin_disconnect(Lin_Tcp *tcp)
{
    if (!tcp) return;

    close(tcp->fd);
    free(tcp);
}

internal Lin_Tcp *
lin_connect(const char *address, u16 port)
{
    // open socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket()");
        return 0;
    }


    // connect
	struct sockaddr_in sv_sockaddr;
	sv_sockaddr.sin_addr.s_addr = inet_addr(address);
	sv_sockaddr.sin_family = AF_INET;
	sv_sockaddr.sin_port = htons(port);

    int connected = connect(fd, (struct sockaddr *)&sv_sockaddr, sizeof(sv_sockaddr));
    if (connected == -1)
    {
        perror("connect()");
        close(fd);
        return 0;
    }


    // make socket non-blocking
    int set_nonblocking = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    if (set_nonblocking == -1)
    {
        printf("error: fcntl failed to set socket non-blocking\n");
        close(fd);
        return 0;
    }


    // finish
    Lin_Tcp *tcp = malloc(sizeof(Lin_Tcp));
    if (!tcp)
    {
        printf("malloc failed\n");
        close(fd);
        return 0;
    }
    tcp->fd = fd;


    return tcp;
}
