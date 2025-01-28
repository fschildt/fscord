#include <os/os.h>
#include <basic/basic.h>
#include <basic/mem_arena.h>
#include <crypto/rsa.h>
#include <crypto/aes_gcm.h>

#include <string.h>

#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>

#include <sys/socket.h>


// Todo: bulletproof all cases


typedef struct {
    AesGcmKey aes_key_client_encrypt; // also means ..._server_decrypt
    AesGcmKey aes_key_client_decrypt; // also means ..._server_encrypt
    u8 client_random[512];
} HandshakeRequest;

typedef struct {
    u8 client_random[512];
} HandshakeResponse;


struct OSNetSecureStream {
    OSNetSecureStreamStatus status;

    EVP_PKEY *rsa_key;
    AesGcmKey aes_key_encrypt;
    AesGcmKey aes_key_decrypt;
    AesGcmIv aes_iv; // only one direction is initialized and advanced per stream

    MemArena arena;
    u8 arena_mem[1408];

    OSNetStream *stream; // Sadly this will go to another memory page, but I'll accept that.
};

typedef struct {
    size_t max_secure_stream_count;
    void *secure_streams;
    size_t free_secure_stream_count;
    OSNetSecureStream **free_secure_streams;
} OSNetSecureStreamsData;


internal_var OSNetSecureStreamsData s_data;


void
os_net_secure_streams_init(MemArena *arena, size_t max_stream_count)
{
    s_data.max_secure_stream_count = max_stream_count;
    s_data.secure_streams = mem_arena_push(arena, max_stream_count * sizeof(OSNetSecureStream));
    memset(s_data.secure_streams, 0, max_stream_count*sizeof(OSNetSecureStream));
    for (size_t i = 0; i < max_stream_count; i++) {
        OSNetSecureStream *secure_stream = &s_data.secure_streams[i];
        secure_stream->status = OS_NET_SECURE_STREAM_DISCONNECTED;
        mem_arena_init(&secure_stream->arena, secure_stream->arena_mem, sizeof(secure_stream->arena_mem));
    }

    s_data.free_secure_stream_count = max_stream_count;
    s_data.free_secure_streams = mem_arena_push(arena, max_stream_count * sizeof(OSNetSecureStream*));
    for (size_t i = 0; i < max_stream_count; i++) {
        s_data.free_secure_streams[i] = &s_data.secure_streams[i];
    }

    os_net_streams_init(arena, max_stream_count);
}

OSNetSecureStreamStatus
os_net_secure_stream_get_status(OSNetSecureStream *stream)
{
    return stream->status;
}


internal_fn void
os_net_secure_stream_free(OSNetSecureStream *secure_stream)
{
    s_data.free_secure_streams[s_data.free_secure_stream_count++] = secure_stream;
}

internal_fn OSNetSecureStream *
os_net_secure_stream_alloc()
{
    if (unlikely(s_data.free_secure_stream_count == 0)) {
        return 0;
    }

    OSNetSecureStream *secure_stream = s_data.free_secure_streams[--s_data.free_secure_stream_count];
    return secure_stream;
}


b32
os_net_secure_stream_recv(OSNetSecureStream *secure_stream, void *buff, size_t size)
{
    if (secure_stream->status == OS_NET_SECURE_STREAM_DISCONNECTED) {
        return false;
    }

    AesGcmHeader header;
    if (!os_net_stream_recv(secure_stream->stream, &header, sizeof(header))) {
        return false;
    }

    void *ciphertext = mem_arena_push(&secure_stream->arena, size);
    if (!os_net_stream_recv(secure_stream->stream, ciphertext, size)) {
        mem_arena_reset(&secure_stream->arena);
        return false;
    }

    b32 decrypted = aes_gcm_decrypt(&secure_stream->aes_key_decrypt,
                                    &secure_stream->aes_iv,
                                    buff, ciphertext, size,
                                    header.tag, header.tag_len);
    mem_arena_reset(&secure_stream->arena);

    return decrypted;
}

b32
os_net_secure_stream_send(OSNetSecureStream *secure_stream, void *buff, size_t size)
{
    if (secure_stream->status != OS_NET_SECURE_STREAM_CONNECTED) {
        return false;
    }

    AesGcmHeader *aes_header = mem_arena_push(&secure_stream->arena, sizeof(*aes_header));
    void *ciphertext = mem_arena_push(&secure_stream->arena, size);
    b32 encrypted = aes_gcm_encrypt(&secure_stream->aes_key_encrypt, &secure_stream->aes_iv,
                                    ciphertext, buff, size,
                                    aes_header->tag, aes_header->tag_len);
    if (!encrypted) {
        secure_stream->status = OS_NET_SECURE_STREAM_DISCONNECTED;
        mem_arena_reset(&secure_stream->arena);
        return false;
    }

    b32 sent = os_net_stream_send(secure_stream->stream, buff, size);
    if (!sent) {
        secure_stream->status = OS_NET_SECURE_STREAM_DISCONNECTED;
        mem_arena_reset(&secure_stream->arena);
        return false;
    }

    mem_arena_reset(&secure_stream->arena);
    return true;
}

int
os_net_secure_stream_get_fd(OSNetSecureStream *secure_stream)
{
    return os_net_stream_get_fd(secure_stream->stream);
}

void
os_net_secure_stream_close(OSNetSecureStream *secure_stream)
{
    os_net_stream_close(secure_stream->stream);
    secure_stream->status = OS_NET_SECURE_STREAM_DISCONNECTED;
}

OSNetSecureStream *
os_net_secure_stream_listen(u16 port, EVP_PKEY *rsa_key_pri)
{
    OSNetSecureStream *secure_stream = os_net_secure_stream_alloc();
    if (!secure_stream) {
        return 0;
    }
    secure_stream->status = OS_NET_SECURE_STREAM_DISCONNECTED;
    secure_stream->rsa_key = rsa_key_pri;
    secure_stream->stream = os_net_stream_listen(port);
    if (!secure_stream->stream) {
        os_net_secure_stream_free(secure_stream);
        return 0;
    }
    return secure_stream;
}

OSNetSecureStream *
os_net_secure_stream_accept(OSNetSecureStream *listener)
{
    OSNetSecureStream *secure_stream = os_net_secure_stream_alloc();
    if (!secure_stream) {
        return 0;
    }

    secure_stream->stream = os_net_stream_accept(listener->stream);
    if (!secure_stream->stream) {
        os_net_secure_stream_free(secure_stream);
        return 0;
    }


    u8 encrypted_req[sizeof(HandshakeRequest)];
    if (!os_net_stream_recv(secure_stream->stream, encrypted_req, sizeof(encrypted_req))) {
        os_net_secure_stream_free(secure_stream);
        return 0;
    }
    HandshakeRequest req;
    if (!rsa_decrypt(listener->rsa_key, &req, encrypted_req, sizeof(req))) {
        os_net_secure_stream_free(secure_stream);
        return 0;
    }


    HandshakeResponse resp;
    memcpy(resp.client_random, req.client_random, sizeof(req.client_random));
    if (!os_net_stream_send(secure_stream->stream, &resp, sizeof(resp))) {
        os_net_secure_stream_free(secure_stream);
        return 0;
    }


    secure_stream->status = OS_NET_SECURE_STREAM_CONNECTED;
    secure_stream->rsa_key = 0;
    aes_gcm_key_copy(&secure_stream->aes_key_encrypt, &req.aes_key_client_decrypt);
    aes_gcm_key_copy(&secure_stream->aes_key_decrypt, &req.aes_key_client_encrypt);
    if (!aes_gcm_iv_init(&secure_stream->aes_iv)) {
        os_net_secure_stream_free(secure_stream);
        return 0;
    }


    return secure_stream;
}

OSNetSecureStream *
os_net_secure_stream_connect(char *address, u16 port, EVP_PKEY *rsa_server_pub)
{
    OSNetSecureStream *secure_stream = os_net_secure_stream_alloc();

    secure_stream->stream = os_net_stream_connect(address, port);
    if (!secure_stream->stream) {
        os_net_secure_stream_free(secure_stream);
        return 0;
    }

    secure_stream->rsa_key = rsa_server_pub;
    if (!aes_gcm_key_init_random(&secure_stream->aes_key_encrypt)) {
        os_net_secure_stream_free(secure_stream);
        return 0;
    }
    if (!aes_gcm_key_init_random(&secure_stream->aes_key_decrypt)) {
        os_net_secure_stream_free(secure_stream);
        return 0;
    }
    if (!aes_gcm_iv_init(&secure_stream->aes_iv)) {
        os_net_secure_stream_free(secure_stream);
        return 0;
    }


    // Todo: Do the Handshake in another thread to prevent stalling the program.


    secure_stream->stream = os_net_stream_connect(address, port);
    if (!secure_stream->stream) {
        os_net_secure_stream_free(secure_stream);
        return 0;
    }


    /* Request */

    HandshakeRequest *req = mem_arena_push(&secure_stream->arena, sizeof(*req));
    aes_gcm_key_copy(&req->aes_key_client_encrypt, &secure_stream->aes_key_encrypt);
    aes_gcm_key_copy(&req->aes_key_client_decrypt, &secure_stream->aes_key_decrypt);
    if (RAND_bytes(req->client_random, sizeof(req->client_random)) != 1) {
        mem_arena_reset(&secure_stream->arena);
        os_net_secure_stream_free(secure_stream);
        return 0;
    }

    // encrypt
    void *encrypted_req = mem_arena_push(&secure_stream->arena, sizeof(*req));
    if (!rsa_encrypt(rsa_server_pub, encrypted_req, req, sizeof(*req))) {
        mem_arena_reset(&secure_stream->arena);
        os_net_secure_stream_free(secure_stream);
        return 0;
    }

    // send
    if (!os_net_stream_send(secure_stream->stream, encrypted_req, sizeof(*req))) {
        mem_arena_reset(&secure_stream->arena);
        os_net_secure_stream_free(secure_stream);
        return 0;
    }


    /* Response */

    HandshakeResponse *resp = mem_arena_push(&secure_stream->arena, sizeof(*resp));
    if (!os_net_stream_recv(secure_stream->stream, resp, sizeof(*resp))) {
        mem_arena_reset(&secure_stream->arena);
        os_net_secure_stream_free(secure_stream);
        return 0;
    }

    // verify response
    b32 success = 1;
    for (size_t i = 0; i < sizeof(req->client_random); i++) {
        if (req->client_random[i] != resp->client_random[i]) {
            success = 0;
            // don't break here to prevent a timing attack? xd
        }
    }
    if (!success) {
        mem_arena_reset(&secure_stream->arena);
        os_net_secure_stream_free(secure_stream);
        return 0;
    }

    secure_stream->status = OS_NET_SECURE_STREAM_CONNECTED;
    secure_stream->rsa_key = rsa_server_pub;
    return secure_stream;
}

