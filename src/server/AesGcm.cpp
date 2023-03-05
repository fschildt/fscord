#include <AesGcm.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <openssl/evp.h>
#include <openssl/rand.h>

bool AesGcm::Init(uint8_t *key, int key_size) {
    if (!(key_size == 32) && !(key_size == 24) && !(key_size == 16)) {
        printf("Aes::Init got invalid key_size\n");
        return false;
    }

    m_KeySize = key_size;
    memcpy(m_Key, key, key_size);
    return true;
}

bool AesGcm::Encrypt(uint8_t *from, uint8_t *to, int len, uint8_t *iv_out, int iv_len, uint8_t *tag_out, int tag_out_len) {
    if (RAND_bytes(iv_out, iv_len) <= 0) {
        return false;
    }


    EVP_CIPHER_CTX *ctx;
    int encrypted_len;
    int tmp_len;


    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        printf("EVP_CIPHER_CTX_new() failed\n");
        return false;
    }


    // encrypt
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), 0, m_Key, iv_out) != 1) {
        printf("EVP_EncryptInit_ex() failed\n");
        return false;
    }

    if (EVP_EncryptUpdate(ctx, to, &tmp_len, from, len) != 1) {
        printf("EVP_EncryptUpdate failed\n");
        return false;
    }
    encrypted_len = tmp_len;

    if (EVP_EncryptFinal_ex(ctx, to + encrypted_len, &tmp_len) != 1) {
        printf("EVP_EncryptFinal_ex failed\n");
        return false;
    }
    encrypted_len += tmp_len;

    // get tag
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tag_out_len, tag_out) != 1) {
        printf("EVP_CIPHER_CTX_ctrl failed\n");
        return false;
    }

    printf("AesGcm::Encrypt:\n"
           "\t key = [%d, %d, ...]\n"
           "\t  iv = [%d, %d, ...]\n"
           "\t tag = [%d, %d, ...]\n"
           "\tsize = %d\n"
           "\tfrom = [%d, %d, ...]\n"
           "\t  to = [%d, %d, ...]\n",
           m_Key[0], m_Key[1],
           iv_out[0], iv_out[1],
           tag_out[0], tag_out[1],
           encrypted_len,
           from[0], from[1],
           to[0], to[1]);

    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool AesGcm::Decrypt(uint8_t *ciphertext, int32_t ciphertext_len,
                     uint8_t *plaintext, // plaintext_len = ciphertext_len
                     uint8_t *iv,  int32_t iv_size,
                     uint8_t *tag, int32_t tag_size) {

    printf("AesGcm::Decrypt:\n"
           "\t key = [%d, %d, ...]\n"
           "\t  iv = [%d, %d, ...]\n"
           "\t tag = [%d, %d, ...]\n"
           "\tsize = %d\n"
           "\tfrom = [%d, %d, ...]\n",
           m_Key[0], m_Key[1],
           iv[0], iv[1],
           tag[0], tag[1],
           ciphertext_len,
           ciphertext[0], ciphertext[1]);

    EVP_CIPHER_CTX *ctx;
    int plaintext_len;
    int tmp_len;

    if (!(ctx = EVP_CIPHER_CTX_new()))
    {
        printf("EVP_CIPHER_CTX_new() failed\n");
        return false;
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), 0, m_Key, iv) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        printf("EVP_DecryptInit_ex() failed\n");
        return false;
    }

    if (EVP_DecryptUpdate(ctx, plaintext, &tmp_len, ciphertext, ciphertext_len) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        printf("EVP_DecryptUpdate() failed\n");
        return false;
    }
    plaintext_len = tmp_len;
    printf("\t  to = [%d, %d, ...]\n",
           plaintext[0], plaintext[1]);

    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag_size, tag) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        printf("EVP_CIPHER_CTX_ctrl (GCM_SET_TAG) failed\n");
        return false;
    }

    if (EVP_DecryptFinal_ex(ctx, plaintext + plaintext_len, &tmp_len) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        printf("EVP_DecryptFinal_ex failed\n");
        return false;
    }
    plaintext_len += tmp_len;

    if (plaintext_len != ciphertext_len)
    {
        EVP_CIPHER_CTX_free(ctx);
        printf("aes_gcm_decrypt failed: plaintext_len != ciphertext_len\n");
        return false;
    }
    EVP_CIPHER_CTX_free(ctx);

    return true;
}
