#include <os/os.h>
#include <client/server_connection.h>
#include <basic/basic.h>
#include <client/fscord.h>
#include <crypto/rsa.h>
#include <messages/messages.h>
#include <string.h>

#include <poll.h> // Todo: use os/os.h


typedef struct {
    volatile ServerConnectionStatus status;
    volatile u32 secure_stream_id;

    char address[128];
    u16 port;
    EVP_PKEY *server_rsa_pub;

    MemArena send_arena;
    u8 send_arena_mem[MESSAGES_MAX_PACKAGE_SIZE];

    MemArena recv_arena;
    u8 recv_arena_mem[MESSAGES_MAX_PACKAGE_SIZE];
} ServerConnection;


internal_var ServerConnection s_server_connection;
internal_var struct Fscord *s_fscord;


void
send_c2s_chat_message(String32 *content)
{
    MemArena *send_arena = &s_server_connection.send_arena;


    C2S_ChatMessage *chat_message = mem_arena_push(send_arena, sizeof(C2S_ChatMessage));

    String32 *content_copy = string32_create_from_string32(send_arena, content);
    chat_message->content = (String32*)((u8*)content_copy - send_arena->size_used);

    chat_message->header.type = C2S_CHAT_MESSAGE;
    chat_message->header.size = send_arena->size_used;


    os_net_secure_stream_send(s_server_connection.secure_stream_id, send_arena->memory, send_arena->size_used);
    mem_arena_reset(send_arena);
}


void
send_c2s_login(String32 *username, String32 *password)
{
    MemArena *send_arena = &s_server_connection.send_arena;


    C2S_Login *login = mem_arena_push(send_arena, sizeof(C2S_Login));

    String32 *username_copy = string32_create_from_string32(send_arena, username);
    login->username = (String32*)((u8*)username_copy - send_arena->size_used);

    String32 *password_copy = string32_create_from_string32(send_arena, password);
    login->password = (String32*)((u8*)password_copy - send_arena->size_used);

    login->header.type = C2S_LOGIN;
    login->header.size = send_arena->size_used;


    os_net_secure_stream_send(s_server_connection.secure_stream_id, send_arena->memory, send_arena->size_used);
    mem_arena_reset(send_arena);
}


internal_fn void
handle_s2c_user_update()
{
    S2C_UserUpdate *user_update = (S2C_UserUpdate*)s_server_connection.recv_arena.memory;
    user_update->username = (String32*)((u8*)user_update + (size_t)user_update->username);

    if (user_update->status == S2C_USER_UPDATE_ONLINE) {
        session_add_user(s_fscord->session, user_update->username);
    } else {
        session_rm_user(s_fscord->session, user_update->username);
    }
}


internal_fn void
handle_s2c_chat_message()
{
    S2C_ChatMessage *chat_message = (S2C_ChatMessage*)s_server_connection.recv_arena.memory;
    chat_message->username = (String32*)((u8*)chat_message + (size_t)chat_message->username);
    chat_message->content  = (String32*)((u8*)chat_message + (size_t)chat_message->content);

    Time time = {chat_message->epoch_time_seconds, chat_message->epoch_time_nanoseconds};

    session_add_chat_message(s_fscord->session, time, chat_message->username, chat_message->content);
}


internal_fn void
handle_s2c_login()
{
    S2C_Login *login = (S2C_Login*)&s_server_connection.recv_arena.memory;
    if (login->login_result == S2C_LOGIN_SUCCESS) {
        s_server_connection.status = SERVER_CONNECTION_ESTABLISHED;
    }
}


internal_fn b32
handle_s2c()
{
    MemArena *recv_arena = &s_server_connection.recv_arena;
    i64 size_recvd;


    MessageHeader *header = mem_arena_push(recv_arena, sizeof(*header));
    size_recvd = os_net_secure_stream_recv(s_server_connection.secure_stream_id, header, sizeof(*header));
    if (size_recvd < 0) {
        printf("unhandled error: size_recvd < 0\n");
        return false;
    }


    // Todo: verify body size
    size_t body_size = header->size - sizeof(MessageHeader);
    void *body = mem_arena_push(recv_arena, body_size);
    size_recvd = os_net_secure_stream_recv(s_server_connection.secure_stream_id, body, body_size);
    if (size_recvd < 0) {
        printf("unhandled error: size_recvd < 0\n");
        return false;
    }


    switch (header->type) {
        case S2C_LOGIN:        handle_s2c_login();        break;
        case S2C_CHAT_MESSAGE: handle_s2c_chat_message(); break;
        case S2C_USER_UPDATE:  handle_s2c_user_update();  break;
        InvalidDefaultCase;
    }


    return true;
}


b32
server_connection_handle_events()
{
    struct pollfd pollfd;
    pollfd.fd = os_net_secure_stream_get_fd(s_server_connection.secure_stream_id);
    pollfd.events = POLLIN;

    for (;;) {
        int ret = poll(&pollfd, 1, 0);
        if (ret == -1) {
            printf("poll error\n");
            return false; // Todo: handle error
        }
        else if (ret == 0) {
            return true; // do nothing
        }
        else if (pollfd.revents & (POLLERR)) {
            printf("poll POLLERR\n");
            return false; // Todo: handle err
        }
        else if (pollfd.revents & POLLHUP) {
            printf("poll POLLHUP\n");
            return false; // Todo: handle hup
        }
        else if (pollfd.revents & POLLIN) {
            b32 handled = handle_s2c();
            if (!handled) {
                return false;
            }
            continue;
        }
    }

    return true;
}


void
server_connection_terminate()
{
    os_net_secure_stream_close(s_server_connection.secure_stream_id);
    s_server_connection.secure_stream_id = OS_NET_SECURE_STREAM_ID_INVALID;
    s_server_connection.status = SERVER_CONNECTION_NOT_ESTABLISHED;
}



// TODO: THIS THREADING IS TEMPORARY. WE WANT TO USE OS/OS.H THREADS

internal_fn void *
server_connection_establish_runner(void *data)
{
    char *address = s_server_connection.address;
    u16 port = s_server_connection.port;
    EVP_PKEY *rsa = s_server_connection.server_rsa_pub;

    u32 secure_stream_id = os_net_secure_stream_connect(address, port, rsa);
    if (secure_stream_id == OS_NET_SECURE_STREAM_ID_INVALID) {
        s_server_connection.status = SERVER_CONNECTION_NOT_ESTABLISHED;
        pthread_exit(0);
    }

    s_server_connection.secure_stream_id = secure_stream_id;
    s_server_connection.status = SERVER_CONNECTION_ESTABLISHED;

    pthread_exit(0);
}

void
server_connection_establish(char *address, u16 port, EVP_PKEY *server_rsa_pub)
{
    s_server_connection.status = SERVER_CONNECTION_ESTABLISHING;

    strncpy(s_server_connection.address, address, strlen(address));
    s_server_connection.port = port;
    s_server_connection.server_rsa_pub = server_rsa_pub;

    pthread_t tid;
    int err = pthread_create(&tid, 0, server_connection_establish_runner, 0);
    if (err != 0) {
        printf("pthread_create error in server_connection_establish\n");
        s_server_connection.status = SERVER_CONNECTION_NOT_ESTABLISHED;
    }

    return;
}


ServerConnectionStatus
server_connection_get_status()
{
    return s_server_connection.status;
}


void
server_connection_create(MemArena *arena, struct Fscord *fscord)
{
    s_fscord = fscord;
    s_server_connection.status = SERVER_CONNECTION_NOT_ESTABLISHED;
    s_server_connection.secure_stream_id = OS_NET_SECURE_STREAM_ID_INVALID;

    mem_arena_init(&s_server_connection.send_arena,
                   s_server_connection.send_arena_mem,
                   sizeof(s_server_connection.send_arena_mem));

    mem_arena_init(&s_server_connection.recv_arena,
                   s_server_connection.recv_arena_mem,
                   sizeof(s_server_connection.recv_arena_mem));
}

