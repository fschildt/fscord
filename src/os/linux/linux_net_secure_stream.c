#include <os/os.h>
#include <basic/basic.h>
#include <basic/mem_arena.h>
#include <crypto/rsa.h>
#include <crypto/aes_gcm.h>

#include <string.h>

#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>


// Todo: bulletproof all cases
// Todo: change OS_NET_STREAM_ID_INVALID to 0 so it's easier to write and check?
// Todo: have b32 or flag with 'has_error' so user code can close them


typedef struct {
    AesGcmKey aes_key_client_encrypt; // also means ..._server_decrypt
    AesGcmKey aes_key_client_decrypt; // also means ..._server_encrypt
    AesGcmIv  aes_iv;
    u8 client_random[512];
} CM_Handshake;

typedef struct {
    u8 client_random[512];
} SM_Handshake;


typedef struct {
    int fd;
    OSNetSecureStreamStatus status;

    EVP_PKEY *rsa_key;

    b32          recv_aes_header_initialized;
    size_t       recv_aes_payload_size_processed;
    AesGcmHeader recv_aes_decrypt_header; // received with each message
    AesGcmKey    recv_aes_decrypt_key; // established during handshake
    AesGcmIv     recv_aes_decrypt_iv;  // established during handshake; advanced for each message
    u8           recv_buff[1408];

    AesGcmKey    send_aes_encrypt_key;
    AesGcmIv     send_aes_encrypt_iv;
    size_t       send_buff_size;
    u8           send_buff[1408];
} OSNetSecureStream;


internal_var u32 s_max_secure_stream_count;
internal_var OSNetSecureStream *s_secure_streams;

internal_var u32 s_free_id_count;
internal_var u32 *s_free_ids;



OSNetSecureStreamStatus
os_net_secure_stream_get_status(u32 id)
{
    return s_secure_streams[id].status;
}


i64
os_net_secure_stream_recv(u32 id, void *buff, size_t size)
{
    OSNetSecureStream *secure_stream = &s_secure_streams[id];
    if (secure_stream->status == OS_NET_SECURE_STREAM_DISCONNECTED) {
        return 0;
    }

    i64 size_processed = 0;
    i64 size_to_recv;
    i64 recvd_size;
    void *src;
    void *dest;

    while (size) {
        // 1) recv aes header
        if (!secure_stream->recv_aes_header_initialized) {
            size_to_recv = sizeof(AesGcmHeader);

            recvd_size = recv(secure_stream->fd,
                              &secure_stream->recv_aes_decrypt_header,
                              sizeof(secure_stream->recv_aes_decrypt_header),
                              MSG_PEEK);
            if (recvd_size < size_to_recv) {
                // Todo: error handling?
                return 0;
            }

            recvd_size = recv(secure_stream->fd,
                              &secure_stream->recv_aes_decrypt_header,
                              sizeof(secure_stream->recv_aes_decrypt_header),
                              MSG_WAITALL);
            if (recvd_size < size_to_recv) {
                // Todo: error handling?
                return 0;
            }

            secure_stream->recv_aes_header_initialized = true;
        }


        // 2) recv aes payload
        size_t payload_remain = secure_stream->recv_aes_decrypt_header.payload_size
                              - secure_stream->recv_aes_payload_size_processed;
        size_to_recv = MIN(size, payload_remain);
        size_to_recv = MIN(size_to_recv, ARRAY_COUNT(secure_stream->recv_buff));

        dest = secure_stream->recv_buff;

        recvd_size = os_net_secure_stream_recv(secure_stream->fd, dest, size_to_recv);
        if (recvd_size < 0) {
            // Todo: error handling?
            return 0;
        }


        // 3) decrypt to buff
        src  = secure_stream->recv_buff;
        dest = buff + size_processed;
        b32 decrypted = aes_gcm_decrypt(&secure_stream->recv_aes_decrypt_key,
                                        &secure_stream->recv_aes_decrypt_iv,
                                        dest, src, recvd_size,
                                        secure_stream->recv_aes_decrypt_header.tag,
                                        secure_stream->recv_aes_decrypt_header.tag_len);
        if (!decrypted) {
            // Todo: error handling?
            return false;
        }


        // 4) clean up
        secure_stream->recv_aes_payload_size_processed += recvd_size;
        if (secure_stream->recv_aes_payload_size_processed == secure_stream->recv_aes_decrypt_header.payload_size) {
            secure_stream->recv_aes_payload_size_processed = 0;
            secure_stream->recv_aes_header_initialized = false;
        }

        size_processed += recvd_size;
        size -= recvd_size;
    }

    return size_processed;
}

b32
os_net_secure_stream_send(u32 id, void *buff, size_t size)
{
    OSNetSecureStream *secure_stream = &s_secure_streams[id];
    if (secure_stream->status != OS_NET_SECURE_STREAM_CONNECTED) {
        return false;
    }


    // Note: We're sending a new aes header every time, because the recipient
    //       needs the tag to verify each part of the message

    size_t size_processed = 0;
    while (size_processed < size) {
        AesGcmHeader *aes_header = (AesGcmHeader*)secure_stream->send_buff;

        void *src = buff + size_processed;
        void *payload = secure_stream->send_buff + sizeof(*aes_header);
        i64 payload_size = MIN(size, ARRAY_COUNT(secure_stream->send_buff) - sizeof(*aes_header));

        b32 encrypted = aes_gcm_encrypt(&secure_stream->send_aes_encrypt_key,
                                        &secure_stream->send_aes_encrypt_iv,
                                        payload, src, payload_size,
                                        aes_header->tag, aes_header->tag_len);
        if (!encrypted) {
            return false;
        }

        size_t size_to_send = sizeof(*aes_header) + payload_size;
        i64 sent_size = send(secure_stream->fd, secure_stream->send_buff, size, 0);
        if (sent_size != size_to_send) {
            return false;
        }

        size_processed += payload_size;
    }

    return true;
}

int
os_net_secure_stream_get_fd(u32 id)
{
    OSNetSecureStream *secure_stream = &s_secure_streams[id];
    return secure_stream->fd;
}


internal_fn void
os_net_secure_stream_free(u32 id)
{
    s_free_ids[s_free_id_count] = id;
    s_free_id_count += 1;
}


internal_fn u32
os_net_secure_stream_alloc()
{
    assert(s_free_id_count > 0);
    if (s_free_id_count == 0) {
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }

    u32 id = s_free_ids[s_free_id_count-1];
    s_free_id_count -= 1;

    return id;
}


void
os_net_secure_stream_close(u32 id)
{
    OSNetSecureStream *secure_stream = &s_secure_streams[id];

    close(secure_stream->fd);
    memset(secure_stream, 0, sizeof(*secure_stream));

    os_net_secure_stream_free(id);
}

u32
os_net_secure_stream_listen(u16 port, EVP_PKEY *rsa_pri)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket()");
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }

    int enable_reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable_reuse, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        close(fd);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }

    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind()");
        close(fd);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }

    int backlog = 128;
    if (listen(fd, backlog) == -1) {
        perror("listen()");
        close(fd);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }


    u32 id = os_net_secure_stream_alloc();

    OSNetSecureStream *secure_stream = &s_secure_streams[id];
    secure_stream->fd = fd;
    secure_stream->status = OS_NET_SECURE_STREAM_CONNECTED;
    secure_stream->rsa_key = rsa_pri;


    return id;
}


u32
os_net_secure_stream_accept(u32 listener_id)
{
    OSNetSecureStream *listener = &s_secure_streams[listener_id];


    // Todo: Do the connect/handshake in another thread to prevent stalling the program.
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);

    printf("accepting...\n");
    int fd = accept(listener->fd, (struct sockaddr*)&addr, &addr_size);
    if (fd == -1) {
        printf("accept() failed\n");
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }
    printf("accepted\n");

    u32 secure_stream_id = os_net_secure_stream_alloc();
    if (secure_stream_id == OS_NET_SECURE_STREAM_ID_INVALID) {
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }

    OSNetSecureStream *secure_stream = &s_secure_streams[secure_stream_id];
    secure_stream->fd = fd;
    secure_stream->status = OS_NET_SECURE_STREAM_HANDSHAKING;


    u8 encrypted_cm_handshake[sizeof(CM_Handshake)];
    i64 recvd_size = recv(secure_stream->fd, encrypted_cm_handshake, sizeof(encrypted_cm_handshake), MSG_WAITALL);
    if (recvd_size != sizeof(encrypted_cm_handshake)) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }

    CM_Handshake cm_handshake;
    if (!rsa_decrypt(listener->rsa_key, &cm_handshake, encrypted_cm_handshake, sizeof(encrypted_cm_handshake))) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }


    SM_Handshake sm_handshake;
    assert(sizeof(sm_handshake.client_random) == sizeof(cm_handshake.client_random));
    memcpy(sm_handshake.client_random, cm_handshake.client_random, sizeof(cm_handshake.client_random));
    i64 sent_size = send(secure_stream->fd, &sm_handshake, sizeof(sm_handshake), 0);
    if (sent_size != sizeof(sm_handshake)) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }


    secure_stream->status = OS_NET_SECURE_STREAM_CONNECTED;
    secure_stream->rsa_key = 0;
    aes_gcm_key_copy(&secure_stream->send_aes_encrypt_key, &cm_handshake.aes_key_client_decrypt);
    aes_gcm_key_copy(&secure_stream->recv_aes_decrypt_key, &cm_handshake.aes_key_client_encrypt);
    // Todo: i kind of guessed that it's send_aes_encrypt_iv; verify it!
    if (!aes_gcm_iv_init(&secure_stream->send_aes_encrypt_iv)) {
        os_net_secure_stream_free(secure_stream_id);
        return 0;
    }


    return secure_stream_id;
}

u32
os_net_secure_stream_connect(char *address, u16 port, EVP_PKEY *server_rsa_pub)
{
    // Todo: Do the connect/handshake in another thread to prevent stalling the program.


    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        printf("cant open socket\n");
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }

    struct sockaddr_in target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(port);
    target_addr.sin_addr.s_addr = inet_addr(address);
    if (connect(fd, (struct sockaddr*)&target_addr, sizeof(target_addr)) == -1) {
        printf("connect failed\n");
        close(fd);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }


    u32 secure_stream_id = os_net_secure_stream_alloc();
    if (secure_stream_id == OS_NET_SECURE_STREAM_ID_INVALID) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }

    OSNetSecureStream *secure_stream = &s_secure_streams[secure_stream_id];
    secure_stream->fd = fd;
    secure_stream->rsa_key = server_rsa_pub;

    if (!aes_gcm_key_init_random(&secure_stream->recv_aes_decrypt_key)) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }
    if (!aes_gcm_key_init_random(&secure_stream->send_aes_encrypt_key)) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }
    if (!aes_gcm_iv_init(&secure_stream->send_aes_encrypt_iv)) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }




    /* Request */

    CM_Handshake cm_handshake;
    aes_gcm_key_copy(&cm_handshake.aes_key_client_encrypt, &secure_stream->send_aes_encrypt_key);
    aes_gcm_key_copy(&cm_handshake.aes_key_client_decrypt, &secure_stream->recv_aes_decrypt_key);
    if (RAND_bytes(cm_handshake.client_random, sizeof(cm_handshake.client_random)) != 1) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_STREAM_ID_INVALID;
    }


    // encrypt
    assert(ARRAY_COUNT(secure_stream->send_buff) >= sizeof(cm_handshake));

    void *encrypted_req = secure_stream->send_buff;
    if (!rsa_encrypt(server_rsa_pub, encrypted_req, &cm_handshake, sizeof(cm_handshake))) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_STREAM_ID_INVALID;
    }


    // send
    i64 sent_size = send(secure_stream->fd, encrypted_req, sizeof(cm_handshake), 0);
    if (sent_size != sizeof(cm_handshake)) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_STREAM_ID_INVALID;
    }


    /* Response */

    SM_Handshake sm_handshake;
    i64 recvd_size = recv(secure_stream->fd, &sm_handshake, sizeof(sm_handshake), 0);
    if (recvd_size != sizeof(sm_handshake)) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_STREAM_ID_INVALID;
    }

    // verify response
    assert(sizeof(cm_handshake.client_random) == sizeof(sm_handshake.client_random));
    if (memcmp(cm_handshake.client_random, sm_handshake.client_random, sizeof(cm_handshake.client_random)) != 0) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_STREAM_ID_INVALID;
    }

    secure_stream->status = OS_NET_SECURE_STREAM_CONNECTED;
    secure_stream->rsa_key = server_rsa_pub;
    return secure_stream_id;
}

void
os_net_secure_streams_init(MemArena *arena, size_t max_count)
{
    s_max_secure_stream_count = max_count;
    s_secure_streams = mem_arena_push(arena, max_count * sizeof(OSNetSecureStream));
    memset(s_secure_streams, 0, max_count * sizeof(OSNetSecureStream));

    s_free_id_count = max_count;
    s_free_ids = mem_arena_push(arena, max_count * sizeof(u32));
    for (size_t i = 0; i < max_count; i++) {
        s_free_ids[i] = i;
    }
}

