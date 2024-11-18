#include <os/os.h>
#include <basic/mem_arena.h>
#include <net/net_secure_stream.h>

typedef enum {
    NET_EVENT_MSG
} NetEventType;

typedef struct {
    NetEventType type;
} NetEvent;

typedef struct {
    NetSecureStream *secure_stream;
} Netman;

Netman *netman_create(MemArena *arena);
void netman_establish_session(Netman *netman, char *address, u16 port, EVP_PKEY *serer_pubkey, EVP_PKEY *client_pubkey, OSWorkQueue *work_queue);

