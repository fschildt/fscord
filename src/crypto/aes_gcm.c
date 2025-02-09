#include <crypto/aes_gcm.h>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <string.h>

b32
aes_gcm_key_init_random(AesGcmKey *key)
{
    int success = RAND_priv_bytes(key->buff, sizeof(key->buff));
    return success == 1;
}

b32
aes_gcm_iv_init(AesGcmIv *iv)
{
    if (RAND_priv_bytes(iv->salt, sizeof(iv->salt)) < 0) {
        return false;
    }

    u32 *counter = (u32*)iv->counter;
    assert(sizeof(*counter) == sizeof(iv->counter));
    *counter = 0;

    return true;
}

void
aes_gcm_iv_advance(AesGcmIv *iv)
{
    u32 *counter = (u32*)iv->counter;
    assert(sizeof(*counter) == sizeof(iv->counter));

    *counter += 1;
    if (*counter == U32_MAX) {
        exit(0); // Todo: make new keys
    }
}

b32
aes_gcm_decrypt(AesGcmKey *key, AesGcmIv *iv,
                void *plaintext, void *ciphertext, i32 ciphertext_len,
                u8 *tag, i32 tag_len)
{
    EVP_CIPHER_CTX *ctx;
    i32 plaintext_len = 0;
    i32 len = 0;

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        printf("EVP_CIPHER_CTX_new() failed\n");
        return false;
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), 0, 0, 0) != 1) {
        printf("EVP_DecryptInit_ex() failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, 0) != 1) {
        printf("EVP_CIPHER_CTX_ctrl setting iv length failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if(EVP_DecryptInit_ex(ctx, 0, 0, key->buff, (u8*)iv) != 1) {
        printf("EVP_DecryptInit_ex with key and iv failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1) {
        printf("EVP_DecryptUpdate() failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plaintext_len += len;

    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag_len, tag) != 1) {
        printf("EVP_CIPHER_CTX_ctrl setting tag failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) != 1) {
        ERR_print_errors_fp(stderr);
        printf("EVP_DecryptFinal_ex failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plaintext_len += len;

    if (plaintext_len != ciphertext_len) {
        printf("aes_gcm_decrypt failed: decrypted_len != len\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    EVP_CIPHER_CTX_free(ctx);
    aes_gcm_iv_advance(iv);
    return true;
}

b32
aes_gcm_encrypt(AesGcmKey *key, AesGcmIv *iv,
                void *ciphertext, void *plaintext, i32 plaintext_len,
                u8 *tag_out, i32 tag_out_len)
{
    EVP_CIPHER_CTX *ctx;
    i32 ciphertext_len = 0;
    i32 len = 0;

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        printf("EVP_CIPHER_CTX_new() failed\n");
        return false;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), 0, 0, 0) != 1) {
        printf("EVP_EncryptInit_ex() failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, 0) != 1) {
        printf("EVP_CIPHER_CTX_ctrl setting iv length failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), 0, key->buff, (u8*)iv) != 1) {
        printf("EVP_EncryptInit_ex() failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1) {
        printf("EVP_EncryptUpdate failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len += len;

    if (EVP_EncryptFinal_ex(ctx, plaintext + len, &len) != 1) {
        printf("EVP_EncryptFinal_ex failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len += len;

    if (ciphertext_len != plaintext_len) {
        printf("aes_gcm_encrypt failed: ciphertext_len = %d, plaintext_len = %d\n", ciphertext_len, plaintext_len);
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tag_out_len, tag_out) != 1) {
        printf("EVP_CIPHER_CTX_ctrl failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    EVP_CIPHER_CTX_free(ctx);
    aes_gcm_iv_advance(iv);
    return true;
}

