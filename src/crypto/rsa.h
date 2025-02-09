#include <basic/basic.h>
#include <basic/mem_arena.h>
#include <openssl/evp.h>

// Note: The rsa input size is calculated by modulus_size - 2*hash_len - 2
//       The rsa input size for 4096bit key and SHA-512 = 512-(2*64)-2 = 382 bytes
//       The rsa output size is always equal to the modulus_size (key_size)

// Note: The arena in rsa_create_via_file only uses it temporarily and cleans it up.

EVP_PKEY *rsa_create_via_file(MemArena *arena, char *filepath, b32 is_public);
EVP_PKEY *rsa_create_via_gen(i32 keysize_in_bits);
EVP_PKEY *rsa_create_via_pem(char *pem, size_t pem_len, b32 is_public);
void      rsa_destroy(EVP_PKEY *key);

b32       rsa_get_pem(EVP_PKEY *key, char *dest, size_t dest_size);
b32       rsa_encrypt(EVP_PKEY *key, void *dest, void *src, size_t size);
b32       rsa_decrypt(EVP_PKEY *key, void *dest, void *src, size_t size);

