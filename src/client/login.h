#ifndef LOGIN_H
#define LOGIN_H

#include <basic/string32.h>
#include <os/os.h>
#include <crypto/rsa.h>
#include <crypto/aes_gcm.h>
#include <client/string32_handles.h>

struct Fscord;

typedef struct {
    b32 is_username_active; // else servername is selected

    String32Buffer *username;
    String32Buffer *servername;
    String32Handle warning;

    EVP_PKEY *rsa_client_pri;
    EVP_PKEY *rsa_server_pub;
} Login;

Login *login_create(MemArena *arena);
void login_process_event(struct Fscord *fscord, OSEvent *event);
void login_draw(struct Fscord *fscord);

#endif // LOGIN_H
