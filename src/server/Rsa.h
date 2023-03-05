#include <openssl/evp.h>

class Rsa {
public:
    bool Init(const char *key_as_string);

    bool Decrypt(void *from, void *to, size_t to_len);
    // from_len = m_KeySizeInBytes by convention

public:
    int GetKeySizeInBytes();

private:
    EVP_PKEY *m_Key;
    EVP_PKEY_CTX *m_KeyContext;
    int m_KeySizeInBytes;
};
