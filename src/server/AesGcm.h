// Conventions:
// - key_len = 32 bytes (256bit)
// - iv_len = 12 bytes
// - tag_len = 16 bytes
// - from_len = to_len

#pragma once

#include <stdint.h>

class AesGcm
{
public:
    bool Init(uint8_t *key, int key_size);

    bool Encrypt(uint8_t *from, uint8_t *to, int len,
                 uint8_t *iv_out, int iv_out_len,
                 uint8_t *tag_out, int tag_out_len);

    bool Decrypt(uint8_t *ciph, int32_t ciph_len,
                 uint8_t *plaintext,
                 uint8_t *iv, int32_t iv_size,
                 uint8_t *tag, int32_t tag_size);

public: // nocheckin
    int m_TagSize;
    int m_IvSize; // iv = nonce
    int m_KeySize;
    uint8_t m_Key[32];
};
