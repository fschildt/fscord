#include <Rsa.h>

#include <openssl/pem.h>

bool Rsa::Init(const char *key_path) {
    if (m_Key) {
        printf("Rsa::Init(%s) failed; already initted\n", key_path);
        return false;
    }


    // receive key
    FILE *f = fopen(key_path, "r");
    if (!f) {
        printf("fopen with key_path %s failed\n", key_path);
        return false;
    }

    EVP_PKEY *key = PEM_read_PrivateKey(f, 0, 0, 0);
    if (!key) {
        printf("PEM_read_PrivateKey with key_path %s failed\n", key_path);
        fclose(f);
        return false;
    }


    // init key context (for encrypt/decrypt operations)
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(key, 0);
    if (!ctx) {
        printf("EVP_PKEY_CTX_new failed\n");
        EVP_PKEY_free(key);
        return true;
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        printf("EVP_PKEY_encrypt_init failed\n");
        EVP_PKEY_free(key);
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        printf("EVP_PKEY_CTX_set_rsa_padding failed\n");
        EVP_PKEY_free(key);
        EVP_PKEY_CTX_free(ctx);
        return false;
    }


    // 
    m_Key = key;
    m_KeyContext = ctx;
    m_KeySizeInBytes = EVP_PKEY_size(key);
    return true;
}

int Rsa::GetKeySizeInBytes() {
    return m_KeySizeInBytes;
}

bool Rsa::Decrypt(void *from, void *to, size_t to_len) {
    size_t from_len = m_KeySizeInBytes;

    size_t to_len_avail;

    if (1 != EVP_PKEY_decrypt(m_KeyContext,
                              0, &to_len_avail,
                              (unsigned char *)from, from_len)) {
        printf("EVP_PKEY_decrypt failed: to_len_avail not available\n");
        return false;
    }

    if (1 != EVP_PKEY_decrypt(m_KeyContext,
                              (unsigned char *)to, &to_len_avail,
                              (unsigned char *)from, from_len)) {
        printf("EVP_PKEY_decrypt outlen not what it should've been\n");
        return false;
    }

    if (to_len_avail != to_len) {
        printf("EVP_PKEY_decrypt failed: to_len_avail = %zu, to_len = %zu\n",
               to_len_avail, to_len);
        return false;
    }

    return true;
}
