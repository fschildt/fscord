#ifndef LOGIN_H
#define LOGIN_H

#include <basic/string32.h>
#include <os/os.h>
#include <crypto/rsa.h>
#include <crypto/aes_gcm.h>
#include <client/string32_handles.h>

struct Fscord;

typedef struct {
    b32 is_username_active;

    b32 is_trying_to_login;
    b32 is_c2s_login_sent;

    String32Buffer *username;
    String32Buffer *servername;
    String32Handle warning;

    EVP_PKEY *rsa_client_pri;
    EVP_PKEY *rsa_server_pub;
} Login;

Login *login_create(MemArena *arena, struct Fscord *fscord);
void login_update_login_attempt(Login *login);
void login_process_login_result(Login *login, b32 result);
void login_process_event(Login *login, OSEvent *event);
void login_draw(Login *login);

#endif // LOGIN_H
