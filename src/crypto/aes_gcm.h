#ifndef AES_GCM_H
#define AES_GCM_H

#include <basic/basic.h>

typedef struct {
    u8 buff[32]; // aes-256 -> 256 bit = 32 bytes
} AesGcmKey;

// information from: https://www.rfc-editor.org/rfc/rfc5288#section-3
typedef struct {
    u32 nonce_salt;    // 4 bytes, aka salt, init'd during handshake
    u64 nonce_counter; // 8 bytes, counter, up to 18 quintillion messages
} AesGcmIv;

typedef struct {
    u32 tag_len;
    u8 tag[16];
    i32 payload_size;
} AesGcmHeader;


// Authentication Tag:
// - is sent with every message
// - tag_len: in bits {96, 104, 112, 120, 128}, in bytes [12, 16]
// - https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38d.pdf
//   --> DANGER: max. 2^32 invocations with 64bit tag and 2^15 = 32kB packets


b32  aes_gcm_key_init_random(AesGcmKey *key);
void aes_gcm_key_copy(AesGcmKey *dest, AesGcmKey *src);

b32  aes_gcm_iv_init(AesGcmIv *iv);
void aes_gcm_iv_advance(AesGcmIv *iv);

b32 aes_gcm_encrypt(AesGcmKey *key, AesGcmIv *iv,
                    u8 *dest, u8 *src, i32 len,
                    u8 *tag_out, i32 tag_out_len);
b32 aes_gcm_decrypt(AesGcmKey *key, AesGcmIv *iv,
                    u8 *dest, u8 *src, i32 len,
                    u8 *tag, i32 tag_len);


#endif // AES_GCM_H
