#include <ClientManager.h>

#include <Rsa.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

bool ClientManager::Init(Rsa *rsa, int listening_fd) {
    // we allocate this once and forget about it
    int memory_size = KILOBYTES(64);
    uint8_t *memory = (uint8_t*)malloc(memory_size);
    if (!memory) {
        return false;
    }
    MemoryArena arena;
    arena.Init(memory, memory_size);


    // allocate and init arenas that deal with network packages
    int max_package_size = FSCN_MAX_PACKAGE_SIZE;
    m_ArenaIo.Init(arena.Push(max_package_size), max_package_size);
    m_ArenaIn.Init(arena.Push(max_package_size), max_package_size);
    m_ArenaOut.Init(arena.Push(max_package_size), max_package_size);


    // allocate and init clients
    assert(FSCN_MAX_USERS <= INT16_MAX-1);
    int client_count_max = FSCN_MAX_USERS;
    int clients_size = client_count_max * sizeof(Client);
    Client *clients = (Client*)arena.Push(clients_size);
    for (int i = 0; i < client_count_max; i++) {
        clients->status = CLIENT_DISCONNECTED;
        clients->fd = -1;
    }


    // allocate and init free client indices
    int indices_size = client_count_max * sizeof(int16_t);
    int16_t *free_client_indices = (int16_t*)arena.Push(indices_size);
    for (int16_t i = 0; i < client_count_max; i++) {
        free_client_indices[i] = client_count_max - i - 1;
    }

    // init epoll manager
    if (!m_EpollManager.Init() ||
        !m_EpollManager.AddFd(listening_fd, (void*)&m_ListeningFd)) {
        return false;
    }

    printf("arena size_used = %d, size_max = %d\n\n", arena.SizeUsed, arena.SizeMax);
    m_ListeningFd = listening_fd;
    m_Rsa = rsa;
    m_FreeClientIndicesAvail = client_count_max;
    m_FreeClientIndicesMax = client_count_max;
    m_FreeClientIndices = free_client_indices;
    m_Clients = clients;
    return true;
}

void ClientManager::Run() {
    int max_events = 512;
    struct epoll_event events[max_events];

    for (;;) {
        int event_count = m_EpollManager.GetEvents(events, max_events);
        if (event_count == -1) {
            return;
        }

        for (int i = 0; i < event_count; i++) {
            struct epoll_event *event = events + i;

            int *listening_fd = (int*)event->data.ptr;
            Client *client = (Client*)event->data.ptr;

            if (listening_fd == &m_ListeningFd) {
                if (!(event->events & EPOLLIN)) {
                    printf("error: listening fd has another event than EPOLLIN\n");
                    return;
                }

                AddClient();
            } else {
                if (!(event->events & EPOLLIN)) {
                    printf("error: client fd has another event than EPOLLIN\n");
                    RemoveClient(client);
                    continue;
                }

                HandleSocketInput(client);
            }
        }
    }
}

void ClientManager::AddClient() {
    if (m_FreeClientIndicesAvail == 0) {
        return;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int client_fd = accept(m_ListeningFd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_fd < 0) {
        perror("accept()");
        return;
    }

    int16_t client_index = m_FreeClientIndices[m_FreeClientIndicesAvail-1];
    Client *client = &m_Clients[client_index];
    if (!m_EpollManager.AddFd(client_fd, client)) {
        close(client_fd);
        return;
    }

    m_FreeClientIndicesAvail--;
    client->status = CLIENT_HANDSHAKE_PENDING;
    client->fd = client_fd;
    client->name_len = 0;
}

void ClientManager::RemoveClient(Client *client) {
    if (client->fd == -1) {
        return;
    }


    // remove user
    m_EpollManager.RemoveFd(client->fd);
    close(client->fd);
    client->status = CLIENT_DISCONNECTED;
    client->fd = -1;
    int16_t index = (int16_t)(client - m_Clients);
    m_FreeClientIndices[m_FreeClientIndicesAvail++] = index;


    // notify everyone
    m_ArenaOut.Reset();
    FSCN_S_User_Status *status = (FSCN_S_User_Status*)m_ArenaOut.Push(sizeof(*status));
    status->type = FSCN_S_USER_STATUS;
    status->status = FSCN_S_USER_STATUS_OFFLINE;
    status->name_len = client->name_len;
    m_ArenaOut.Append((uint8_t*)client->name, client->name_len);

    EncryptAndSendToEveryoneExcept(client);


    // debug print
    printf("removed user: ");
    for (int i = 0; i < status->name_len; i++) {
        putchar(client->name[i]);
    }
    putchar('\n');
}

void ClientManager::HandleSocketInput(Client *client) {
    // @Quality: Make a prethreaded pool, and have them wait for work

    switch (client->status) {
        case CLIENT_DISCONNECTED: {
            return;
        }

        case CLIENT_HANDSHAKE_PENDING: {
            printf("\nprocessing CLIENT_HANDSHAKE_PENDING\n");

            // recv handshake request
            m_ArenaIo.Reset();
            if (!RecvIoData(client->fd, m_Rsa->GetKeySizeInBytes())) {
                RemoveClient(client);
                return;
            }
            printf("\tEncrypted FSCN_Handshake_Request:\n"
                   "\t\treq = [%d, %d, ...]\n",
                   m_ArenaIo.Memory[0], m_ArenaIo.Memory[1]);

            // decrypt
            FSCN_C_Handshake *req = (FSCN_C_Handshake*)m_ArenaIn.Memory;
            if (!DecryptIoData(m_Rsa, sizeof(*req))) {
                printf("\tDecryptIoData failed\n");
                RemoveClient(client);
                return;
            }
            printf("\tDecrypted FSCN_Handshake_Request:\n"
                   "\t\treq->aes_gcm_key   = [%d, %d, ...]\n"
                   "\t\treq->client_random = [%d, %d, ...]\n",
                   req->aes_gcm_key[0], req->aes_gcm_key[1],
                   req->client_random[0], req->client_random[1]);
            if (!client->aes_gcm.Init(req->aes_gcm_key, sizeof(req->aes_gcm_key))) {
                printf("\tDecryptIoData failed\n");
                RemoveClient(client);
                return;
            }

            // make response
            m_ArenaOut.Reset();
            FSCN_S_Handshake *resp = (FSCN_S_Handshake*)m_ArenaOut.Push(sizeof(*resp));
            resp->type = FSCN_S_HANDSHAKE;
            memcpy(resp->client_random, req->client_random, sizeof(req->client_random));
            if (!EncryptOutData(&client->aes_gcm)) {
                printf("\tEncryptOutData failed\n");
                RemoveClient(client);
                return;
            }


            // send response
            if (!SendIoData(client->fd)) {
                printf("\tSendIoData failed\n");
                RemoveClient(client);
                return;
            }
            m_ArenaIo.Reset();

            client->status = CLIENT_LOGIN_PENDING;
            printf("done.\n");
            return;
        } break;

        case CLIENT_LOGIN_PENDING: {
            printf("\nprocessing CLIENT_LOGIN_PENDING\n");

            // recv package
            if (!RecvAndDecrypt(client->fd, &client->aes_gcm)) {
                printf("\tRecvAndDecrypt failed\n");
                RemoveClient(client);
                return;
            }

            // check header
            FSCN_C_Login *c_login = (FSCN_C_Login*)m_ArenaIn.Memory;
            if (m_ArenaIn.SizeUsed < sizeof(*c_login) ||
                c_login->username_len < 0 ||
                c_login->username_len > FSCN_MAX_USERNAME_LEN ||
                m_ArenaIn.SizeUsed != sizeof(*c_login) + c_login->username_len) {
                printf("\tinvalid package, removing client\n");
                RemoveClient(client);
                return;
            }

            // update client state
            const char *username = (const char*)(c_login + 1);
            int16_t username_len = c_login->username_len;
            memcpy(client->name, username, username_len);
            client->name_len = username_len;
            client->status = CLIENT_LOGGED_IN;

            // send data to new client
            DoLoginResponse(client, FSCN_S_LOGIN_SUCCESS);
            SendUserlistTo(client);

            // send data to all other clients
            m_ArenaOut.Reset();
            FSCN_S_User_Status *status = (FSCN_S_User_Status*)m_ArenaOut.Push(sizeof(*status));
            status->type = FSCN_S_USER_STATUS;
            status->status = FSCN_S_USER_STATUS_ONLINE;
            status->name_len = client->name_len;
            m_ArenaOut.Append((uint8_t*)client->name, client->name_len);
            EncryptAndSendToEveryoneExcept(client);
            printf("done.\n");
        } break;

        case CLIENT_LOGGED_IN: {
            printf("\nprocessing CLIENT_LOGGED_IN\n");
            if (!RecvAndDecrypt(client->fd, &client->aes_gcm)) {
                printf("\tcould not recv/decrypt full package\n");
                RemoveClient(client);
                return;
            }

            FSCN_C_Message *c_message = (FSCN_C_Message*)m_ArenaIn.Memory;

            // check package
            if (m_ArenaIn.SizeUsed < sizeof(*c_message) ||
                c_message->type != FSCN_C_MESSAGE ||
                c_message->content_len < 0 ||
                c_message->content_len > FSCN_MAX_MESSAGE_LEN ||
                m_ArenaIn.SizeUsed != sizeof(*c_message) + c_message->content_len) {
                printf("\tpackage invalid\n");
                RemoveClient(client);
                return;
            }

            char *c_message_content = (char*)(c_message + 1);

            // prepare package
            m_ArenaOut.Reset();
            FSCN_S_Message *s_message = (FSCN_S_Message*)m_ArenaOut.Push(sizeof(*s_message));
            s_message->type = FSCN_S_MESSAGE;
            s_message->sender_len = client->name_len;
            s_message->content_len = c_message->content_len;
            s_message->epoch_time = time(0);
            m_ArenaOut.Append((uint8_t*)client->name, client->name_len);
            m_ArenaOut.Append((uint8_t*)c_message_content, c_message->content_len);
            EncryptAndSendToEveryone();
            printf("done.\n");
        } break;

        default: {
            printf("switch(client->status) has invalid default\n");
        }
    }
}

void ClientManager::DoLoginResponse(Client *client, int status) {
    MemoryArena *arena_out = &m_ArenaOut;
    arena_out->Reset();

    FSCN_S_Login *resp = (FSCN_S_Login*)arena_out->Push(sizeof(*resp));
    resp->type = FSCN_S_LOGIN;
    resp->status = status;

    if (!EncryptAndSend(client->fd, &client->aes_gcm)) {
        RemoveClient(client);
    }
}

void ClientManager::SendUserlistTo(Client *recipient) {
    m_ArenaOut.Reset();

    FSCN_S_User_Status *status = (FSCN_S_User_Status*)m_ArenaOut.Push(sizeof(*status));
    status->type = FSCN_S_USER_STATUS;
    status->status = FSCN_S_USER_STATUS_ONLINE;
    char *status_name = (char*)m_ArenaOut.Push(FSCN_MAX_USERNAME_LEN);

    int16_t max_clients = m_FreeClientIndicesMax - m_FreeClientIndicesAvail;
    Client *client = m_Clients;
    for (int i = 0; i < max_clients; i++) {
        if (client->status == CLIENT_LOGGED_IN) {
            status->name_len = client->name_len;
            memcpy(status_name, client->name, client->name_len);

            // little hack to avoid work
            m_ArenaOut.SizeUsed = sizeof(*status) + client->name_len;

            if (!EncryptAndSend(recipient->fd, &recipient->aes_gcm)) {
                RemoveClient(recipient);
                break;
            }
        }

        client++;
    }

    m_ArenaOut.Reset();
}


void ClientManager::EncryptAndSendToEveryone() {
    MemoryArena *arena_io = &m_ArenaIo;

    Client *client = m_Clients;
    int16_t max_client_index = m_FreeClientIndicesMax - 1;
    for (int16_t i = 0; i < max_client_index; i++) {
        if (client->status == CLIENT_LOGGED_IN) {
            if (!EncryptAndSend(client->fd, &client->aes_gcm)) {
                RemoveClient(client);
            }
        }
        client++;
    }
}

void ClientManager::EncryptAndSendToEveryoneExcept(Client *except) {
    MemoryArena *arena_io = &m_ArenaIo;

    Client *max_client = m_Clients + (m_FreeClientIndicesMax - 1);
    Client *client = m_Clients;

    while (client < except) {
        if (client->status == CLIENT_LOGGED_IN) {
            if (!EncryptAndSend(client->fd, &client->aes_gcm)) {
                RemoveClient(client);
            }
        }
        client++;
    }
    client++;

    while (client <= max_client) {
        if (client->status == CLIENT_LOGGED_IN) {
            if (!EncryptAndSend(client->fd, &client->aes_gcm)) {
                RemoveClient(client);
            }
        }
        client++;
    }
}


bool ClientManager::EncryptAndSend(int fd, AesGcm *aes) {
    if (fd == -1) {
        return false;
    }

    if (!EncryptOutData(aes)) {
        return false;
    }
    if (!SendIoData(fd)) {
        return false;
    }

    return true;
}

bool ClientManager::RecvAndDecrypt(int fd, AesGcm *aes) {
    if (fd == -1) {
        return false;
    }

    if (!RecvAesPackage(fd)) {
        return false;
    }

    if (!DecryptIoData(aes)) {
        return false;
    }

    return true;
}

bool ClientManager::EncryptOutData(AesGcm *aes) {
    MemoryArena *arena_from = &m_ArenaOut;
    uint8_t *from = arena_from->Memory;
    int from_size = arena_from->SizeUsed;

    MemoryArena *arena_to = &m_ArenaIo;
    arena_to->Reset();
    FSCN_Aes_Header *aes_header = (FSCN_Aes_Header*)arena_to->Push(sizeof(*aes_header));
    uint8_t *payload = arena_to->Push(from_size);

    if (!aes->Encrypt(from, payload, from_size,
                      aes_header->iv_nonce, sizeof(aes_header->iv_nonce),
                      aes_header->auth_tag, sizeof(aes_header->auth_tag))) {
        printf("aes->Encrypted failed\n");
        return false;
    }
    aes_header->payload_size = from_size;
    return true;
}

bool ClientManager::DecryptIoData(AesGcm *aes) {
    // from
    MemoryArena *arena_from = &m_ArenaIo;
    FSCN_Aes_Header *aes_header = (FSCN_Aes_Header*)arena_from->Memory;
    uint8_t *payload = arena_from->Memory + sizeof(*aes_header);
    int payload_size = aes_header->payload_size;

    // to
    MemoryArena *arena_to = &m_ArenaIn;
    arena_to->Reset();
    uint8_t *to = arena_to->Push(aes_header->payload_size);

    // decrypt
    if (!aes->Decrypt(payload, payload_size,
                      arena_to->Memory,
                      aes_header->iv_nonce, sizeof(aes_header->iv_nonce),
                      aes_header->auth_tag, sizeof(aes_header->auth_tag))) {
        printf("aes->Decrypt() failed\n");
        return false;
    }
    return true;
}

bool ClientManager::DecryptIoData(Rsa *rsa, int decrypted_size) {
    MemoryArena *arena_from = &m_ArenaIo;
    MemoryArena *arena_to = &m_ArenaIn;

    arena_to->Reset();
    void *dest = arena_to->Push(decrypted_size);
    if (!rsa->Decrypt(arena_from->Memory, dest, decrypted_size)) {
        return false;
    }

    return true;
}


bool ClientManager::RecvAesPackage(int fd) {
    m_ArenaIo.Reset();
    printf("Receiving Aes Package:\n");

    // recv aes header
    FSCN_Aes_Header *aes_header = (FSCN_Aes_Header*)m_ArenaIo.Memory;
    if (!RecvIoData(fd, sizeof(*aes_header))) {
        return false;
    }

    printf("\tiv_nonce = [%d, %d, ...]\n"
           "\tauth_tag = [%d, %d, ...]\n"
           "\tpayload_size = %d\n",
           aes_header->iv_nonce[0], aes_header->iv_nonce[1],
           aes_header->auth_tag[0], aes_header->auth_tag[1],
           aes_header->payload_size);

    // recv aes payload
    if (!RecvIoData(fd, aes_header->payload_size)) {
        return false;
    }

    return true;
}

bool ClientManager::SendIoData(int fd) {
    MemoryArena *arena_io = &m_ArenaIo;

    printf("sending %d bytes\n", arena_io->SizeUsed);
    ssize_t sent = send(fd, arena_io->Memory, arena_io->SizeUsed, 0);
    if (sent != arena_io->SizeUsed) {
        perror("send()");
        return false;
    }

    return true;
}

bool ClientManager::RecvIoData(int fd, size_t size) {
    // @Quality: use nonblocking sockets
    void *dest = m_ArenaIo.Push(size);
    while (size) {
        ssize_t recvd = recv(fd, dest, size, 0);
        if (recvd <= 0) {
            m_ArenaIo.Reset();
            return false;
        }
        size -= recvd;
    }
    return true;
}
