#include <platform/platform.h>
#include <windows.h>
#include <stdlib.h>

static b32 g_wsa_started;

struct PlatformNetworkStream {
    SOCKET socket;
};

PlatformNetworkStream *platform_network_stream_open_connection() {
    PlatformNetworkStream *stream = (PlatformNetworkStream*)malloc(sizeof(*stream));
    int err;

    // start wsa
    if (!g_wsa_started) {
        WSADATA wsa_data;
        WORD wsa_version = MAKEWORD(2, 2);
        err = WSAStartup(wsa_version, &wsa_data);
        if (err) {
            printf("WSAStartup error %d\n", err);
            return false;
        }
        g_wsa_started = true;
    }

    // open socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("socket() wsa error: %d\n", WSAGetLastError());
        return false;
    }

    // connect
    struct sockaddr_in sa = {};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = inet_addr(address);

    err = connect(sock, (SOCKADDR*)&sa, sizeof(sa));
    if (err) {
        printf("connect() wsa error: %d\n", WSAGetLastError());
        closesocket(sock);
        return false;
    }

    // make socket non-blocking
    u_long io_mode = 1;
    err = ioctlsocket(sock, FIONBIO, &io_mode);
    if (err) {
        printf("ioctlsocket() wsa error: %d\n", WSAGetLastError());
        closesocket(sock);
        return false;
    }

    m_Socket = sock;
    return true;
}

void platform_network_stream_close(PlatformNetworkStream *stream) {
    int err = closesocket(m_Socket);
    if (err) {
        printf("closesocket() wsa error: %d\n", WSAGetLastError());
    }
}

bool platform_network_stream_send(PlatformNetworkStream *stream, void *buff, i32 size) {
    int sent = send(m_Socket, (const char*)buff, size, 0);
    if (sent == SOCKET_ERROR) {
        int error_code = WSAGetLastError();
        printf("send() wsa error = %d\n", error_code);
        return false;
    }
    return true;
}

i64 platform_network_stream_recv(PlatformNetworkStream *stream, void *buff, i64 size) {
    int recvd = recv(m_Socket, (char*)buff, size, 0);
    if (recvd == SOCKET_ERROR) {
        int error_code = WSAGetLastError();
        if (error_code == WSAEWOULDBLOCK) {
            return 0;
        } else {
            printf("recv() wsa error: %d\n", error_code);
            return -1;
        }
    }
    return recvd;
}

