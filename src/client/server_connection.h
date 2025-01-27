#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include <basic/basic.h>
#include <crypto/rsa.h>
#include <os/os.h>
#include <messages/messages.h>

struct Fscord;

typedef enum {
    SERVER_CONNECTION_NOT_ESTABLISHED,
    SERVER_CONNECTION_ESTABLISHED
} ServerConnectionStatus;

void server_connection_create(MemArena *arena, struct Fscord *fscord);
b32  server_connection_establish(char *address, u16 port, EVP_PKEY *server_rsa_pub);
void server_connection_terminate();
void server_connection_handle_events();
ServerConnectionStatus server_connection_get_status();

#endif // SERVER_CONNECTION_H
