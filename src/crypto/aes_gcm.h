#ifndef AES_GCM_H
#define AES_GCM_H

#include <basic/basic.h>

// Note: Aes-256
// - https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38d.pdf
// - DANGER: max. 2^32 invocations!!!

// Note: Aes Key
// - for aes-256 always 32 bytes

// Note: Initialization Vector
// - for aes-256 is always 12 bytes for optimal usage
// - https://www.rfc-editor.org/rfc/rfc5288#section-3

// Note: Authentication Tag
// - is sent with every message
// - is calculated after whole message is encrypted
// - len is preferred 16 bytes for max security


typedef struct {
    u8 buff[32];
} AesGcmKey;

typedef struct {
    u8 salt[8];
    u8 counter[4];
} AesGcmIv;

typedef struct {
    u32 payload_size;
    u8 tag[16];
} AesGcmHeader;


b32  aes_gcm_key_init_random(AesGcmKey *key);

b32  aes_gcm_iv_init(AesGcmIv *iv);
void aes_gcm_iv_advance(AesGcmIv *iv);

b32 aes_gcm_encrypt(AesGcmKey *key, AesGcmIv *iv,
                    u8 *ciphertext, u8 *plaintext, i32 plaintext_len,
                    u8 *tag_out, i32 tag_out_len);
b32 aes_gcm_decrypt(AesGcmKey *key, AesGcmIv *iv,
                    u8 *plaintext, u8 *ciphertext, i32 ciphertext_len,
                    u8 *tag, i32 tag_len);


#endif // AES_GCM_H
