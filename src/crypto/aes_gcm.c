#include <crypto/aes_gcm.h>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string.h>

b32 aes_gcm_key_init_random(AesGcmKey *key) {
    int success = RAND_bytes(key->buff, sizeof(key->buff));
    return success == 1;
}

void aes_gcm_key_copy(AesGcmKey *dest, AesGcmKey *src) {
    memcpy(dest->buff, src->buff, sizeof(src->buff));
}

b32 aes_gcm_iv_init(AesGcmIv *iv) {
    int success = RAND_bytes((void*)&iv->nonce_salt, 4);
    iv->nonce_counter = 0;
    return success == 1;
}

void aes_gcm_iv_advance(AesGcmIv *iv) {
    iv->nonce_counter++;
    if (iv->nonce_counter >= 2U<<31) {
        exit(0);
    }
}

b32 aes_gcm_decrypt(AesGcmKey *key, AesGcmIv *iv,
                    u8 *dest, u8 *src, i32 len,
                    u8 *tag, i32 tag_len)
{
    EVP_CIPHER_CTX *ctx;
    i32 decrypted_len = 0;
    i32 decrypted_len_tmp;

    if (!(ctx = EVP_CIPHER_CTX_new())) {
        printf("EVP_CIPHER_CTX_new() failed\n");
        return false;
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), 0, key->buff, (u8*)iv) != 1) {
        printf("EVP_DecryptInit_ex() failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_DecryptUpdate(ctx, dest, &decrypted_len_tmp, src, len) != 1) {
        printf("EVP_DecryptUpdate() failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    decrypted_len += decrypted_len_tmp;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag_len, tag) != 1) {
        printf("EVP_CIPHER_CTX_ctrl with EVP_CTRL_GCM_SET_TAG failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_DecryptFinal_ex(ctx, dest + decrypted_len, &decrypted_len_tmp) != 1) {
        printf("EVP_DecryptFinal_ex failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    decrypted_len += decrypted_len_tmp;

    if (decrypted_len != len) {
        printf("aes_gcm_decrypt failed: decrypted_len != len\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    EVP_CIPHER_CTX_free(ctx);
    aes_gcm_iv_advance(iv);
    return true;
}

b32 aes_gcm_encrypt(AesGcmKey *key, AesGcmIv *iv,
                    u8 *dest, u8 *src, i32 len,
                    u8 *tag_out, i32 tag_out_len)
{
    EVP_CIPHER_CTX *ctx;
    int encrypted_len = 0;
    int encrypted_len_tmp;

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        printf("EVP_CIPHER_CTX_new() failed\n");
        return false;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), 0, key->buff, (u8*)iv) != 1) {
        printf("EVP_EncryptInit_ex() failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_EncryptUpdate(ctx, dest, &encrypted_len_tmp, src, len) != 1) {
        printf("EVP_EncryptUpdate failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    encrypted_len += encrypted_len_tmp;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tag_out_len, tag_out) != 1) {
        printf("EVP_CIPHER_CTX_ctrl failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_EncryptFinal_ex(ctx, src + encrypted_len, &encrypted_len_tmp) != 1) {
        printf("EVP_EncryptFinal_ex failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    encrypted_len += encrypted_len_tmp;

    if (encrypted_len != len) {
        printf("aes_gcm_encrypt failed: encrypted_len != len\n");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    EVP_CIPHER_CTX_free(ctx);
    aes_gcm_iv_advance(iv);
    return true;
}

