#ifndef FSCORD_MESSAGES_H
#define FSCORD_MESSAGES_H

#include <basic/basic.h>
#include <basic/string.h>
#include <os/os.h>
#include <crypto/aes_gcm.h>

#define MESSAGES_MAX_USER_COUNT 128
#define MESSAGES_MAX_USERNAME_LEN 32
#define MESSAGES_MAX_MESSAGE_LEN 1024
#define MESSAGES_MAX_PACKAGE_SIZE 1408 // 1408 fits well in ethernet frames


enum {
    MSG_TYPE_LOGIN_REQUEST,
    MSG_TYPE_LOGIN_RESPONSE,
    MSG_TYPE_ONLINE_STATUS,
    MSG_TYPE_CHAT_MESSAGE,
};

enum {
    MSG_LOGIN_RESULT_SUCCESS,
    MSG_LOGIN_RESULT_ERROR
};

enum {
    MSG_ONLINE_STATUS_ONLINE,
    MSG_ONLINE_STATUS_OFFLINE,
};

typedef struct {
    u32 message_type;
    u32 password_len;
    u32 username_len;
} MsgLoginRequest;

typedef struct {
    u32 message_type;
    u32 login_result;
} MsgLoginResponse;

typedef struct {
    u32 message_type;
    u32 status;
    u32 username_len;
} MsgOnlineStatus;

typedef struct {
    u32 message_type;
    u32 sender_username_len;
    u32 content_len;
    i64 epoch_time_seconds;
    i64 epoch_time_nanoseconds;
} MsgChatMessage;

void msg_init(MemArena *arena, OSNetSecureStream *secure_stream);
void msg_login_response(u32 login_result);
void msg_online_status(String32 *username, u32 online_status);
void msg_login_request(String32 *password, String32 *username);
void msg_chat_message(String32 *content);

#endif // FSCORD_MESSAGES_H
