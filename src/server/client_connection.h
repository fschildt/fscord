#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <basic/basic.h>
#include <basic/string32.h>
#include <os/os.h>

typedef struct {
    String32Buffer *username; // Todo: 'cursor' field unused, but the struct is easy to use. Keep it?
    OSNetSecureStream *secure_stream;
} ClientConnection;

void client_connection_deinit_and_free(i16 id);
i16  client_connection_alloc_and_init(OSNetSecureStream *secure_stream);
b32  client_connections_create(MemArena *arena);


#endif // CLIENT_CONNECTION_H
