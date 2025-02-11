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
#include <fcntl.h>


typedef struct {
    // space left for 512 - 64*2 - 2 = 382 for 4096-bit rsa and SHA-512
    AesGcmKey aes_key_client_encrypt; // 32 bytes
    AesGcmIv  aes_iv_client_encrypt;  // 12 bytes

    AesGcmKey aes_key_client_decrypt; // 32 bytes
    AesGcmIv  aes_iv_client_decrypt;  // 12 bytes

    // space left for 382 - 2*32 - 2*12 = 294 for 4096-bit rsa and SHA-512
    u8 client_random[256];
} CM_Handshake;

typedef struct {
    u8 client_random[256];
} SM_Handshake;


typedef struct {
    int fd;
    OSNetSecureStreamStatus status;

    EVP_PKEY *rsa_key;

    AesGcmKey    recv_aes_key;    // recvd with CM_Handshake
    AesGcmIv     recv_aes_iv;     // recvd_with CM_Handshake
    AesGcmHeader recv_aes_header; // recvd with each aes message
    b32          recv_aes_header_initialized;
    size_t       recv_buff_size_used;
    u8           recv_buff[1408];

    AesGcmKey    send_aes_key; // recvd with CM_Handshake
    AesGcmIv     send_aes_iv;  // recvd with CM_Handshake
    size_t       send_buff_size_used;
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


// Todo: This restricts the aes payload size. Be able to deal with greater sizes.
i64
os_net_secure_stream_recv(u32 id, void *buff, size_t size)
{
    OSNetSecureStream *secure_stream = &s_secure_streams[id];
    if (secure_stream->status == OS_NET_SECURE_STREAM_DISCONNECTED) {
        return -1;
    }

    if (size > sizeof(secure_stream->recv_buff)) {
        return -1;
    }


    i64 size_to_recv;
    i64 size_recvd;


    // 1) recv aes header
    if (!secure_stream->recv_aes_header_initialized) {
        void *dest = secure_stream->recv_buff + secure_stream->recv_buff_size_used;
        size_to_recv = sizeof(AesGcmHeader) - secure_stream->recv_buff_size_used;
        size_recvd = recv(secure_stream->fd, dest, size_to_recv, 0);

        if (size_recvd < 0) {
            return -1;
        }
        else if (size_recvd < size_to_recv) {
            secure_stream->recv_buff_size_used += size_recvd;
            return 0;
        }

        secure_stream->recv_aes_header_initialized = true;
        secure_stream->recv_aes_header = *(AesGcmHeader*)(secure_stream->recv_buff);
        secure_stream->recv_buff_size_used = 0;
    }


    // 2) recv aes payload
    if (secure_stream->recv_aes_header.payload_size > sizeof(secure_stream->recv_buff)) {
        return -1;
    }
    if (secure_stream->recv_aes_header.payload_size != size) {
        return -1;
    }
    size_to_recv = secure_stream->recv_aes_header.payload_size - secure_stream->recv_buff_size_used;
    size_recvd = os_net_secure_stream_recv(secure_stream->fd, secure_stream->recv_buff, size_to_recv);
    if (size_recvd < 0) {
        return -1;
    }
    else if (size_recvd != size_to_recv) {
        secure_stream->recv_buff_size_used += size_recvd;
        return 0;
    }
    secure_stream->recv_buff_size_used += size_recvd;


    // 3) decrypt to buff
    b32 decrypted = aes_gcm_decrypt(&secure_stream->recv_aes_key,
                                    &secure_stream->recv_aes_iv,
                                    buff, secure_stream->recv_buff, secure_stream->recv_buff_size_used,
                                    secure_stream->recv_aes_header.tag,
                                    sizeof(secure_stream->recv_aes_header.tag));
    if (!decrypted) {
        return -1;
    }


    // 4) cleanup
    secure_stream->recv_buff_size_used = 0;
    secure_stream->recv_aes_header_initialized = false;


    return size;
}

b32
os_net_secure_stream_send(u32 id, void *buff, size_t size)
{
    OSNetSecureStream *secure_stream = &s_secure_streams[id];
    if (secure_stream->status != OS_NET_SECURE_STREAM_CONNECTED) {
        return false;
    }


    while (size) {
        AesGcmHeader *aes_header = (AesGcmHeader*)secure_stream->send_buff;
        void *aes_payload = secure_stream->send_buff + sizeof(*aes_header);
        i64 aes_payload_size = MIN(size, sizeof(secure_stream->send_buff) - sizeof(*aes_header));

        aes_header->payload_size = aes_payload_size;
        b32 encrypted = aes_gcm_encrypt(&secure_stream->send_aes_key,
                                        &secure_stream->send_aes_iv,
                                        aes_payload, buff, aes_payload_size,
                                        aes_header->tag, sizeof(aes_header->tag));
        if (!encrypted) {
            return false;
        }

        size_t size_to_send = sizeof(*aes_header) + aes_payload_size;
        i64 size_sent = send(secure_stream->fd, secure_stream->send_buff, size_to_send, 0);
        if (size_sent != size_to_send) {
            return false;
        }

        buff += aes_payload_size;
        size -= aes_payload_size;
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


internal_fn b32
set_socket_nonblocking(int fd)
{
    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0)) < 0) {
        perror("fcntl(F_GETFL)");
        return false;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL)");
        return false;
    }
    return true;
}

u32
os_net_secure_stream_accept(u32 listener_id)
{
    OSNetSecureStream *listener = &s_secure_streams[listener_id];


    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);

    int fd = accept(listener->fd, (struct sockaddr*)&addr, &addr_size);
    if (fd == -1) {
        printf("accept() failed\n");
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }


    u32 secure_stream_id = os_net_secure_stream_alloc();
    if (secure_stream_id == OS_NET_SECURE_STREAM_ID_INVALID) {
        close(fd);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }

    OSNetSecureStream *secure_stream = &s_secure_streams[secure_stream_id];
    secure_stream->fd = fd;
    secure_stream->status = OS_NET_SECURE_STREAM_HANDSHAKING;


    // recv rsa request
    u8 encrypted_cm_handshake[512]; // Todo: use secure_stream->recv_buff
    i64 recvd_size = recv(secure_stream->fd, encrypted_cm_handshake, sizeof(encrypted_cm_handshake), MSG_WAITALL);
    if (recvd_size != sizeof(encrypted_cm_handshake)) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }

    // decrypt rsa request
    CM_Handshake cm_handshake;
    if (!rsa_decrypt(listener->rsa_key, &cm_handshake, encrypted_cm_handshake, sizeof(encrypted_cm_handshake))) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }


    // process request
    memcpy(&secure_stream->recv_aes_key, &cm_handshake.aes_key_client_encrypt, sizeof(cm_handshake.aes_key_client_encrypt));
    memcpy(&secure_stream->recv_aes_iv,  &cm_handshake.aes_iv_client_encrypt,  sizeof(cm_handshake.aes_iv_client_encrypt));
    memcpy(&secure_stream->send_aes_key, &cm_handshake.aes_key_client_decrypt, sizeof(cm_handshake.aes_key_client_decrypt));
    memcpy(&secure_stream->send_aes_iv,  &cm_handshake.aes_iv_client_decrypt,  sizeof(cm_handshake.aes_iv_client_decrypt));


    // prepare aes response
    AesGcmHeader *aes_header = (AesGcmHeader*)secure_stream->send_buff;

    SM_Handshake sm_handshake;
    memcpy(sm_handshake.client_random, cm_handshake.client_random, sizeof(cm_handshake.client_random));

    // encrypt aes response
    if (!aes_gcm_encrypt(&secure_stream->send_aes_key,
                         &secure_stream->send_aes_iv,
                         secure_stream->send_buff + sizeof(*aes_header), &sm_handshake, sizeof(sm_handshake),
                         aes_header->tag, sizeof(aes_header->tag))) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }
    aes_header->payload_size = sizeof(sm_handshake);
    secure_stream->send_buff_size_used = sizeof(*aes_header) + sizeof(sm_handshake);

    // send response
    i64 sent_size = send(secure_stream->fd, secure_stream->send_buff, secure_stream->send_buff_size_used, 0);
    if (sent_size != secure_stream->send_buff_size_used) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }


    if (!set_socket_nonblocking(fd)) {
        close(fd);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }


    secure_stream->status = OS_NET_SECURE_STREAM_CONNECTED;
    return secure_stream_id;
}

u32
os_net_secure_stream_connect(char *address, u16 port, EVP_PKEY *server_rsa_pub)
{
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


    if (!aes_gcm_key_init_random(&secure_stream->send_aes_key)) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }
    if (!aes_gcm_iv_init(&secure_stream->send_aes_iv)) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }
    if (!aes_gcm_key_init_random(&secure_stream->recv_aes_key)) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }
    if (!aes_gcm_iv_init(&secure_stream->recv_aes_iv)) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }


    /* Request */

    // prepare rsa request
    CM_Handshake cm_handshake;
    memcpy(&cm_handshake.aes_key_client_encrypt, &secure_stream->send_aes_key, sizeof(secure_stream->send_aes_key));
    memcpy(&cm_handshake.aes_iv_client_encrypt,  &secure_stream->send_aes_iv,  sizeof(secure_stream->send_aes_iv));
    memcpy(&cm_handshake.aes_key_client_decrypt, &secure_stream->recv_aes_key, sizeof(secure_stream->recv_aes_key));
    memcpy(&cm_handshake.aes_iv_client_decrypt,  &secure_stream->recv_aes_iv,  sizeof(secure_stream->recv_aes_iv));
    if (RAND_bytes(cm_handshake.client_random, sizeof(cm_handshake.client_random)) != 1) {
        printf("RAND_bytes failed at %s:%d", __FILE__, __LINE__);
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }

    // encrypt rsa request
    void *encrypted_req = secure_stream->send_buff;
    if (!rsa_encrypt(server_rsa_pub, encrypted_req, &cm_handshake, sizeof(cm_handshake))) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }

    // send rsa request
    i64 sent_size = send(secure_stream->fd, encrypted_req, 512, 0);
    if (sent_size != 512) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }


    /* Response */

    // recv aes response
    AesGcmHeader *aes_header = (AesGcmHeader*)secure_stream->recv_buff;
    void *aes_payload = secure_stream->recv_buff + sizeof(*aes_header);

    size_t response_size = sizeof(*aes_header) + sizeof(SM_Handshake);

    i64 recvd_size = recv(secure_stream->fd, secure_stream->recv_buff, response_size, 0);
    if (recvd_size != response_size) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }

    // decrypt aes response
    SM_Handshake sm_handshake;
    if (!aes_gcm_decrypt(&secure_stream->recv_aes_key,
                         &secure_stream->recv_aes_iv,
                         &sm_handshake, aes_payload, sizeof(sm_handshake),
                         aes_header->tag, sizeof(aes_header->tag))) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }

    // verify aes response
    assert(sizeof(cm_handshake.client_random) == sizeof(sm_handshake.client_random));
    if (memcmp(cm_handshake.client_random, sm_handshake.client_random, sizeof(cm_handshake.client_random)) != 0) {
        close(fd);
        os_net_secure_stream_free(secure_stream_id);
        return OS_NET_SECURE_STREAM_ID_INVALID;
    }


    if (!set_socket_nonblocking(fd)) {
        close(fd);
        return OS_NET_SECURE_STREAM_ID_INVALID;
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

