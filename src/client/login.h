#ifndef LOGIN_H
#define LOGIN_H

#include <os/os.h>
#include <basic/string.h>
#include <crypto/rsa.h>
#include <crypto/aes_gcm.h>

struct Fscord;

typedef struct {
    b32 is_username_active; // else servername is selected

    String32 *username;
    size_t username_cursor;
    String32 *servername;
    size_t servername_cursor;
    String32 *warning;

    EVP_PKEY *rsa_client_pri;
    EVP_PKEY *rsa_server_pub;
} Login;

Login *login_create(MemArena *arena);
void login_process_event(struct Fscord *fscord, OSEvent *event);
void login_draw(struct Fscord *fscord);

#endif // LOGIN_H
