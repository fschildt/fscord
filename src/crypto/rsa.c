#include <crypto/rsa.h>
#include <os/os.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

b32
rsa_get_pem(EVP_PKEY *key, char *pem, size_t pem_len)
{
    const char *pem_in_bio = 0;

    BIO *bio = BIO_new(BIO_s_mem());
    if (!bio) {
        printf("BIO_new failed\n");
        BIO_free(bio);
        return false;
    }

    if (!PEM_write_bio_PUBKEY(bio, key)) {
        printf("PEM_write_bio_PUBKEY failed\n");
        BIO_free(bio);
        return false;
    }

    long data_size = BIO_get_mem_data(bio, &pem_in_bio);
    if (data_size > pem_len) {
        BIO_free(bio);
        return false;
    }

    memcpy(pem, pem_in_bio, data_size);

    BIO_free(bio);
    return true;
}

b32
rsa_encrypt(EVP_PKEY *key, void *dest, void *src, size_t size)
{
    EVP_PKEY_CTX *ctx;
    
    ctx = EVP_PKEY_CTX_new(key, 0);
    if (!ctx) {
        printf("EVP_PKEY_CTX_new failed\n");
        return false;
    }

    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        printf("EVP_PKEY_encrypt_init failed\n");
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        printf("EVP_PKEY_CTX_set_rsa_padding failed\n");
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    size_t ciphertext_len;
    if (EVP_PKEY_encrypt(ctx, 0, &ciphertext_len, src, size) <= 0) {
        printf("EVP_PKEY_encrypt failed\n");
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    if (EVP_PKEY_encrypt(ctx, dest, &ciphertext_len, src, size) <= 0) {
        printf("EVP_PKEY_encrypt failed\n");
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    printf("rsa encrypted %zu plaintext-bytes to %zu ciphertext-bytes\n", size, ciphertext_len);
    EVP_PKEY_CTX_free(ctx);
    return true;
}

b32
rsa_decrypt(EVP_PKEY *key, void *dest, void *src, size_t size)
{
    return false;
}

void
rsa_destroy(EVP_PKEY *key)
{
    EVP_PKEY_free(key);
}

EVP_PKEY *
rsa_create_via_gen(i32 keysize_in_bits)
{
    EVP_PKEY *key = 0;
    EVP_PKEY_CTX *context;

    context = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, 0);
    if (!context) {
        printf("EVP_PKEY_CTX_new_id failed\n");
        goto end;
    }

    if (EVP_PKEY_keygen_init(context) <= 0) {
        printf("EVP_PKEY_keygen_init failed\n");
        goto end;
    }

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(context, keysize_in_bits) <= 0) {
        printf("EVP_PKEY_CTX_set_rsa_keygen_bits failed\n");
        goto end;
    }

    if (EVP_PKEY_keygen(context, &key) <= 0) {
        printf("EVP_PKEY_keygen failed\n");
        goto end;
    }

end:
    EVP_PKEY_CTX_free(context);
    return key;
}

EVP_PKEY *
rsa_create_via_pem(char *pem, size_t pem_len, b32 is_public)
{
    BIO *bio;
    bio = BIO_new(BIO_s_mem());
    if (!bio) {
        printf("BIO_new failed\n");
        return 0;
    }

    if (BIO_write(bio, pem, pem_len) <= 0) {
        printf("BIO_write failed\n");
        BIO_free(bio);
        return 0;
    }

    EVP_PKEY *key;
    if (is_public) {
        key = PEM_read_bio_PUBKEY(bio, 0, 0, 0);
        if (!key) {
            printf("PEM_read_bio_PUBKEY failed\n");
        }
    } else {
        key = PEM_read_bio_PrivateKey(bio, 0, 0, 0);
        if (!key) {
            printf("PEM_read_bio_PrivateKey failed\n");
        }
    }

    BIO_free(bio);
    return key;
}

EVP_PKEY *
rsa_create_via_file(MemArena *arena, char *filepath, b32 is_public)
{
    EVP_PKEY *key = 0;

    size_t arena_save = mem_arena_save(arena);
    size_t pem_len;
    char *pem = os_read_file_as_string(arena, filepath, &pem_len);
    if (pem) {
        key = rsa_create_via_pem(pem, pem_len, is_public);
    }
    mem_arena_restore(arena, arena_save);

    return key;
}
