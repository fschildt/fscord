#include <basic/basic.h>
#include <basic/mem_arena.h>
#include <basic/string.h>
#include <openssl/evp.h>

// the arena is only used for reading the file, evp_pkey is stored on heap in ssl
EVP_PKEY *rsa_create_via_file(MemArena *arena, char *filepath, b32 is_public);
EVP_PKEY *rsa_create_via_gen(i32 keysize_in_bits);
EVP_PKEY *rsa_create_via_pem(char *pem, size_t pem_len, b32 is_public);
void      rsa_destroy(EVP_PKEY *key);
b32       rsa_get_pem(EVP_PKEY *key, char *dest, size_t dest_size);
b32       rsa_decrypt(EVP_PKEY *key, void *dest, void *src, size_t size);
b32       rsa_encrypt(EVP_PKEY *key, void *dest, void *src, size_t size);

