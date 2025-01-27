#ifndef FSCORD_MESSAGES_H
#define FSCORD_MESSAGES_H

// Todo: Currently every user is identified via his name. Can we change it to an id?

#include <basic/basic.h>
#include <basic/string32.h>
#include <os/os.h>
#include <crypto/aes_gcm.h>


#define MESSAGES_MAX_USER_COUNT 128
#define MESSAGES_MAX_USERNAME_LEN 32
#define MESSAGES_MAX_MESSAGE_LEN 1024
#define MESSAGES_MAX_PACKAGE_SIZE 1408 // 1408 fits well in ethernet frames // Todo: We shouldn't worry about this very much! Change the api!

typedef struct {
    u32 type;
    u32 size;
} MessageHeader;

enum {
    C2S_LOGIN,
    C2S_CHAT_MESSAGE,

    S2C_LOGIN,
    S2C_USER_UPDATE,
    S2C_CHAT_MESSAGE,
};

enum {
    S2C_LOGIN_ERROR,
    S2C_LOGIN_SUCCESS
};

enum {
    S2C_USER_UPDATE_OFFLINE,
    S2C_USER_UPDATE_ONLINE,
};

typedef struct {
    MessageHeader header;
    String32 *username;
    String32 *password;
} C2S_Login;

typedef struct {
    MessageHeader header;
    String32 *content;
} C2S_ChatMessage;

typedef struct {
    MessageHeader header;
    u32 login_result;
} S2C_Login;

typedef struct {
    MessageHeader header;
    u32 status;
    String32 *username;
} S2C_UserUpdate;

typedef struct {
    MessageHeader header;
    i64 epoch_time_seconds;
    i64 epoch_time_nanoseconds;
    String32 *username;
    String32 *content;
} S2C_ChatMessage;


void messages_init(MemArena *arena, OSNetSecureStream *secure_stream);

void c2s_login(String32 *username, String32 *password);
void c2s_chat_message(String32 *content);

void s2c_login(u32 login_result);
void s2c_user_update(String32 *username, u32 online_status);
void s2c_chat_message(String32 *username, String32 *content);


#endif // FSCORD_MESSAGES_H
