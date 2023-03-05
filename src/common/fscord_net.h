#ifndef COMMON_FSCORD_NET_H
#define COMMON_FSCORD_NET_H


#include <stdint.h>

#define FSCN_MAX_USERS 512
#define FSCN_MAX_USERNAME_LEN 32
#define FSCN_MAX_MESSAGE_LEN 128
#define FSCN_MAX_PACKAGE_SIZE 1408 // 1408 fits well in ethernet frames
#define FSCN_CLIENT_RANDOM_SIZE 256


typedef struct
{
    uint8_t iv_nonce[12]; // @Security: beware of the birthday problem
    uint8_t auth_tag[16];
    int32_t payload_size;
} FSCN_Aes_Header;



// packages made in client and sent to server
#define FSCN_C_LOGIN   0
#define FSCN_C_MESSAGE 1

typedef struct
{
    uint8_t aes_gcm_key[32];
    uint8_t client_random[FSCN_CLIENT_RANDOM_SIZE];
} FSCN_C_Handshake;

typedef struct
{
    int16_t type;
    int16_t username_len;
} FSCN_C_Login;

typedef struct
{
    int16_t type;
    int16_t content_len;
} FSCN_C_Message;


// packages made in server and sent to client
#define FSCN_S_HANDSHAKE        0
#define FSCN_S_LOGIN            1
#define FSCN_S_USER_STATUS      2
#define FSCN_S_MESSAGE          3

#define FSCN_S_LOGIN_SUCCESS    0
#define FSCN_S_LOGIN_ERROR_PW   1

#define FSCN_S_USER_STATUS_ONLINE   0
#define FSCN_S_USER_STATUS_OFFLINE  1

typedef struct
{
    int16_t type;
    uint8_t client_random[FSCN_CLIENT_RANDOM_SIZE];
} FSCN_S_Handshake;

typedef struct
{
    int16_t type;
    int16_t status;
} FSCN_S_Login;

typedef struct
{
    int16_t type;
    int16_t status;
    int16_t name_len;
} FSCN_S_User_Status;

typedef struct
{
    int16_t type;
    int16_t sender_len;
    int16_t content_len;
    int64_t epoch_time;
} FSCN_S_Message;



#endif // COMMON_FSCORD_NET_H
