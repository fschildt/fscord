typedef enum
{
    NET_DISCONNECTED,
    NET_HANDSHAKE_PENDING,
    NET_LOGIN_PENDING,
    NET_LOGGED_IN
} Net_Status;

typedef struct
{
    s32 size_used;
    s32 size_total;
    u8 *buffer;
} Net_Package;

typedef struct
{
    Sys_Tcp *tcp;
    Net_Status status;
    const char *last_error;

    u8 aes_gcm_key[32];

    Memory_Arena arena_io;
    Memory_Arena arena_in;
    Memory_Arena arena_out;
} Net;

