#ifndef S2C_SENDER_H
#define S2C_SENDER_H

#include <basic/mem_arena.h>
#include <server/client_connections.h>

void s2c_sender_init(MemArena *arena);

void send_s2c_login(ClientConnection *conn, u32 login_result);
void send_s2c_chat_message(ClientConnection *conn, String32 *username, String32 *content);
void send_s2c_user_udpate(ClientConnection *conn, String32 *username, u32 online_status);

#endif // S2C_SENDER_H
