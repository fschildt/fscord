#include <basic/basic.h>
#include <basic/mem_arena.h>
#include <openssl/evp.h>

// The arena is only used during this function. It reads the file, converts it to EVP_PKEY and gets rid of the file again.
EVP_PKEY *rsa_create_via_file(MemArena *trans_arena, char *filepath, b32 is_public);
EVP_PKEY *rsa_create_via_gen(i32 keysize_in_bits);
EVP_PKEY *rsa_create_via_pem(char *pem, size_t pem_len, b32 is_public);
void      rsa_destroy(EVP_PKEY *key);
b32       rsa_get_pem(EVP_PKEY *key, char *dest, size_t dest_size);
b32       rsa_decrypt(EVP_PKEY *key, void *dest, void *src, size_t size);
b32       rsa_encrypt(EVP_PKEY *key, void *dest, void *src, size_t size);

