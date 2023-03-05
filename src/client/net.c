const char *g_net__error_connect_failed = "connect failed";
const char *g_net__error_handshake_request_failed = "handshake_request failed";
const char *g_net__error_lost_connection = "connection lost";

internal void net_disconnect(Net *net, bool lost_connection);


internal void
net__send(Net *net)
{
    if (net->status == NET_DISCONNECTED)
    {
        return;
    }

    Memory_Arena *arena_io = &net->arena_io;
    printf("sending %zu\n", arena_io->size_used);
    bool sent = sys_send(net->tcp, arena_io->memory, arena_io->size_used);
    if (!sent)
    {
        net_disconnect(net, true);
    }
}

internal bool
net__recv(Net *net, size_t size)
{
    if (net->status == NET_DISCONNECTED)
    {
        return false;
    }


    Memory_Arena *arena_io = &net->arena_io;
    u8 *dest = memory_arena_push(arena_io, size);

    s64 recvd = sys_recv(net->tcp, dest, size);
    if (recvd == -1)
    {
        memory_arena_pop(arena_io, size);
        net_disconnect(net, true);
        return false;
    }
    memory_arena_pop(arena_io, size - recvd);
    if (recvd != size)
    {
        return false;
    }

    return true;
}

internal void
net__encrypt_and_send(Net *net)
{
    Memory_Arena *arena_io = &net->arena_io;
    Memory_Arena *arena_out = &net->arena_out;

    // prepare aes package
    memory_arena_reset(arena_io);
    FSCN_Aes_Header *aes_header = memory_arena_push(arena_io, sizeof(*aes_header));
    void *aes_payload = memory_arena_push(arena_io, arena_out->size_used);


    // encrypt
    if (!aes_gcm_encrypt(arena_out->memory, arena_out->size_used,
                    aes_payload,
                    net->aes_gcm_key, sizeof(net->aes_gcm_key),
                    aes_header->iv_nonce, sizeof(aes_header->iv_nonce),
                    aes_header->auth_tag, sizeof(aes_header->auth_tag)))
    {
        net_disconnect(net, true);
        return;
    }
    aes_header->payload_size = arena_out->size_used;

    printf("Sending aes package (bytes = %zu, key = [%d, %d, ...])\n"
           "\tiv_nonce = [%d, %d, ...]\n"
           "\tauth_tag = [%d, %d, ...]\n"
           "\tpayload_size = %d\n",
           arena_io->size_used, net->aes_gcm_key[0], net->aes_gcm_key[1],
           aes_header->iv_nonce[0], aes_header->iv_nonce[1],
           aes_header->auth_tag[0], aes_header->auth_tag[1],
           aes_header->payload_size);

    net__send(net);
    memory_arena_reset(arena_io);
}

internal bool
net__recv_aes_package_update(Net *net)
{
    Memory_Arena *arena_io = &net->arena_io;

    // recv aes header
    FSCN_Aes_Header *aes_header = (FSCN_Aes_Header*)arena_io->memory;
    if (arena_io->size_used < sizeof(*aes_header))
    {
        size_t size_desired = sizeof(*aes_header) - arena_io->size_used;
        bool recvd_all = net__recv(net, size_desired);
        if (!recvd_all)
        {
            return false;
        }
    }

    // recv aes payload
    size_t payload_size = aes_header->payload_size;
    assert(sizeof(*aes_header) + payload_size != arena_io->size_used);
    if (arena_io->size_used < sizeof(*aes_header) + payload_size)
    {
        size_t size_desired = sizeof(*aes_header) + payload_size - arena_io->size_used;
        bool recvd_all = net__recv(net, size_desired);
        if (!recvd_all)
        {
            return false;
        }
    }

    printf("recv a full aes package\n");
    return true;
}

internal Memory_Arena *
net__decrypt_and_reset_io_data(Net *net)
{
    Memory_Arena *arena_io = &net->arena_io;
    Memory_Arena *arena_in = &net->arena_in;

    // src
    FSCN_Aes_Header *aes_header = arena_io->memory;
    u8 *aes_payload = arena_io->memory + sizeof(*aes_header);

    // dest
    memory_arena_reset(arena_in);
    u8 *dest = memory_arena_push(arena_in, aes_header->payload_size);

    // decrypt
    Memory_Arena *result = arena_in;
    if (!aes_gcm_decrypt(aes_payload, aes_header->payload_size,
                         dest,
                         net->aes_gcm_key, sizeof(net->aes_gcm_key),
                         aes_header->iv_nonce, sizeof(aes_header->iv_nonce),
                         aes_header->auth_tag, sizeof(aes_header->auth_tag)))
    {
        result = 0;
    }

    memory_arena_reset(arena_io);
    return result;
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
            s32 name_len = user_status->name_len;

            if (user_status->status == FSCN_S_USER_STATUS_ONLINE)
            {
                add_user(state, name, name_len);
            }
            else if (user_status->status == FSCN_S_USER_STATUS_OFFLINE)
            {
                remove_user(state, name, name_len);
            }
            else
            {
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

            s64 epoch_time = msg->epoch_time;
            char *sender = arena_in->memory + sizeof(*msg);
            s32 sender_len = msg->sender_len;
            char *content = arena_in->memory + sizeof(*msg) + msg->sender_len;
            s32 content_len = msg->content_len;

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

internal bool
net__do_handshake_request(Net *net)
{
    Memory_Arena *arena_out = &net->arena_out;
    memory_arena_reset(arena_out);


    // prepare request
    FSCN_C_Handshake *req = memory_arena_push(arena_out, sizeof(*req));
    int success;
    success  = RAND_bytes(req->aes_gcm_key, sizeof(req->aes_gcm_key));
    success &= RAND_bytes(req->client_random, sizeof(req->client_random));
    if (!success)
    {
        printf("RAND_bytes failed\n");
        return false;
    }
    memcpy(net->aes_gcm_key, req->aes_gcm_key, sizeof(req->aes_gcm_key));
    printf("FSCN_Handshake_Request:\n"
           "\taes_gcm_key = [%d, %d, ...]\n"
           "\tclient_random = [%d, %d, ...]\n",
           req->aes_gcm_key[0], req->aes_gcm_key[1],
           req->client_random[0], req->client_random[1]);


    // encrypt request
    EVP_PKEY *rsa_sv = pem_to_rsa(g_sv_public_pem, strlen(g_sv_public_pem), true);
    if (!rsa_sv) return false;

    int key_size = EVP_PKEY_size(rsa_sv);
    void *ciphertext = alloca(key_size);
    if (!rsa_encrypt(rsa_sv, req, ciphertext, sizeof(*req)))
    {
        printf("rsa_encrypted failed\n");
        return false;
    }
    EVP_PKEY_free(rsa_sv);

    printf("Encrypted FSCN_Handshake_Request:\n"
           "\t[%d, %d, ...]\n",
           ((uint8_t*)ciphertext)[0], ((uint8_t*)ciphertext)[1]);


    // send request
    if (!sys_send(net->tcp, ciphertext, key_size))
    {
        printf("Could not send FSCN_Handshake_Request\n");
        return false;
    }

    return true;
}


internal const char *
net_get_last_error(Net *net)
{
    const char *error = net->last_error;
    net->last_error = 0;
    return error;
}

internal void
net_send_message(Net *net, char *content, s32 content_len)
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

// Todo: bool expected -> net->error = lost_connection
internal void
net_disconnect(Net *net, bool lost_connection)
{
    sys_disconnect(net->tcp);
    net->tcp = 0;
    net->status = NET_DISCONNECTED;
    if (lost_connection)
    {
        net->last_error = g_net__error_lost_connection;
    }
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
                      const char *username, s32 username_len)
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

internal bool
net_init(Net *net, Memory_Arena *arena)
{
    net->tcp = 0;
    net->status = NET_DISCONNECTED;
    net->last_error = 0;

    // - tcp takes 20-60 bytes (often no options -> 20 bytes)
    // - ipv4 takes 20-60 bytes (often no options -> 20 bytes)
    // - ipv6 takes 40 bytes
    // - ethernet can transmit a payload of 1500 bytes per frame
    // - thus, 1024 + 256 + 128 = 1408 should be a nice value.
    size_t max_package_size = 1408;
    size_t max_payload_size = max_package_size - sizeof(FSCN_Aes_Header);
    
    memory_arena_make_sub_arena(arena, &net->arena_io, max_package_size);
    memory_arena_make_sub_arena(arena, &net->arena_in, max_payload_size);
    memory_arena_make_sub_arena(arena, &net->arena_out, max_payload_size);

    return true;
}
