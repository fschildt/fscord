#include <client/netman.h>
#include <platform/platform.h>

Netman *netman_create(MemArena *arena) {
    Netman *netman = mem_arena_push(arena, Netman);
    netman->secure_network_stream = secure_network_stream_create(arena);
    return netman;
}

typedef struct {
    Netman *netman;
    char *address;
    u16 port;
    Rsa *server_pubkey;
    Rsa *client_pubkey;
} EstablishSessionData;

void netman_establish_session_worker(void *_data) {
    EstablishSessionData *data = _data;
    secure_network_stream_connect(data->netman->secure_network_stream,
                                  data->address,
                                  data->port,
                                  data->server_pubkey,
                                  data->client_pubkey);
}

void netman_establish_session(Netman *netman, char *address, u16 port, Rsa *server_pubkey, Rsa *client_pubkey, PlatformWorkQueue *work_queue) {
    assert(netman->status == NET_STATUS_DISCONNECTED);


    EstablishSessionData data = {
        netman, address, port, server_pubkey, client_pubkey
    };
    PlatformWork work = {netman_establish_session_worker, &data};

    platform_work_queue_push(work_queue, work)
}


internal bool
net__process_package(Net *net, State *state)
{
    Memory_Arena *arena_in = &net->arena_in;

    int16_t *type = arena_in->memory;
    if (arena_in->size_used < sizeof(*type))
    {
        return false;
    }

    switch (*type)
    {
        case FSCN_S_HANDSHAKE:
        {
            printf("\nprocessing FSCN_S_HANDSHAKE\n");
            Memory_Arena *arena_out = &net->arena_out;

            if (net->status != NET_HANDSHAKE_PENDING)
            {
                printf("invalid state to recv FSCN_S_HANDSHAKE_RESPONSE\n");
                return false;
            }

            // get handshake response
            FSCN_S_Handshake *resp = arena_in->memory;
            if (arena_in->size_used != sizeof(*resp))
            {
                printf("invalid FSCN_S_Handshake_Response size\n");
                return false;
            }
            size_t cmp_size = sizeof(resp->client_random);


            // @Hack: verify client random from misused arena_out
            FSCN_C_Handshake *req = arena_out->memory;
            assert(cmp_size == sizeof(req->client_random));
            if (memcmp(resp->client_random, req->client_random, cmp_size) != 0)
            {
                printf("client_random is invalid\n");
                return false;
            }


            // @Hack: get login data from misused arena_out
            u8 *req_payload = arena_out->memory + sizeof(FSCN_C_Handshake);

            FSCN_C_Login login_tmp;
            memcpy(&login_tmp, req_payload, sizeof(FSCN_C_Login));
            req_payload += sizeof(FSCN_C_Login);

            char username_tmp[login_tmp.username_len];
            memcpy(username_tmp, req_payload, login_tmp.username_len);


            // make login package
            memory_arena_reset(arena_out);
            memory_arena_append(arena_out, &login_tmp, sizeof(login_tmp));
            memory_arena_append(arena_out, username_tmp, login_tmp.username_len);


            // encrypt and send login package
            net__encrypt_and_send(net);


            net->status = NET_LOGIN_PENDING;
            printf("done FSCN_S_HANDSHAKE_RESPONSE\n");
            return true;
        }
        break;

        case FSCN_S_LOGIN:
        {
            printf("\nprocessing FSCN_S_LOGIN\n");
            FSCN_S_Login *s_login = arena_in->memory;
            if (sizeof(*s_login) != arena_in->size_used)
            {
                printf("FSCN_S_LOGIN package has invalid size\n");
                return false;
            }

            if (s_login->status == FSCN_S_LOGIN_SUCCESS)
            {
                printf("logged in\n");
                net->status = NET_LOGGED_IN;
                return true;
            }
            else
            {
                net_disconnect(net, true);
                return true;
            }
        }

        case FSCN_S_USER_STATUS:
        {
            printf("\nprocessing FSCN_S_USER_STATUS\n");
            FSCN_S_User_Status *user_status = arena_in->memory;
            size_t size_expected = sizeof(*user_status) + user_status->name_len;
            if (size_expected != arena_in->size_used)
            {
                printf("\trecvd FSCN_S_User_Status has invalid size\n");
                return false;
            }

            char *name = arena_in->memory + sizeof(*user_status);
            i32 name_len = user_status->name_len;

            if (user_status->status == FSCN_S_USER_STATUS_ONLINE) {
                add_user(state, name, name_len);
                playing_sound_start(&state->ps_user_connected);
            }
            else if (user_status->status == FSCN_S_USER_STATUS_OFFLINE) {
                remove_user(state, name, name_len);
                playing_sound_start(&state->ps_user_disconnected);
            }
            else {
                printf("\tinvalid FSCN_USER_STATUS status\n");
                return false;
            }

            printf("done.\n");
            return true;
        }
        break;

        case FSCN_S_MESSAGE:
        {
            printf("\nprocessing FSCN_S_MESSAGE\n");
            FSCN_S_Message *msg = arena_in->memory;
            size_t size_expected = sizeof(*msg) + msg->sender_len + msg->content_len;
            if (size_expected != arena_in->size_used ||
                msg->type != FSCN_S_MESSAGE)
            {
                printf("invalid message, expected FSCN_S_MESSAGE\n");
                return false;
            }

            i64 epoch_time = msg->epoch_time;
            char *sender = arena_in->memory + sizeof(*msg);
            i32 sender_len = msg->sender_len;
            char *content = arena_in->memory + sizeof(*msg) + msg->sender_len;
            i32 content_len = msg->content_len;

            add_message(state, sender, sender_len, content, content_len, epoch_time);
            printf("done.\n");
            return true;
        }
        break;

        default:
        {
            printf("Package invalid\n");
            return false;
        }
    }
}

internal void
net_send_message(Net *net, char *content, i32 content_len)
{
    Memory_Arena *arena_out = &net->arena_out;
    memory_arena_reset(arena_out);

    FSCN_C_Message *msg = memory_arena_push(arena_out, sizeof(FSCN_C_Message));
    msg->type = FSCN_C_MESSAGE;
    msg->content_len = content_len;
    char *content_dest = memory_arena_push(arena_out, content_len);
    memcpy(content_dest, content, content_len);

    net__encrypt_and_send(net);
}


internal void
net_update(Net *net, State *state)
{
    if (net->status == NET_DISCONNECTED)
    {
        return;
    }

    for (;;)
    {
        bool recvd_aes_package = net__recv_aes_package_update(net);
        if (!recvd_aes_package) break;

        bool decrypted = net__decrypt_and_reset_io_data(net);
        if (!decrypted)
        {
            printf("could not decrypt a package???\n");
            break;
        }

        bool processed = net__process_package(net, state);
        if (!processed)
        {
            printf("could not process a package???\n");
            break;
        }
    }
}

internal void
net_connect_and_login(Net *net, const char *address, u16 port,
                      const char *username, i32 username_len)
{
    assert(net->status == NET_DISCONNECTED);

    // @Quality: sys_connect can take a lot of time. We don't want to block here!
    // Fix: Make a thread and poll for result every frame
    Sys_Tcp *tcp = sys_connect(address, port);
    if (!tcp)
    {
        printf("connect failed\n");
        net->last_error = g_net__error_connect_failed;
        printf("net->last_error = %s\n", net->last_error);
        return;
    }
    net->tcp = tcp;

    if (!net__do_handshake_request(net))
    {
        printf("net__do_handshake_request failed\n");
        net->last_error = g_net__error_handshake_request_failed;
        sys_disconnect(tcp);
    }

    // @Hack: append login data to arena_out so we can verify the responded client random from there
    Memory_Arena *arena_out = &net->arena_out;
    FSCN_C_Login *login = memory_arena_push(arena_out, sizeof(*login));
    login->type = FSCN_C_LOGIN;
    login->username_len = username_len;
    memory_arena_append(arena_out, username, username_len);

    net->status = NET_HANDSHAKE_PENDING;
}

